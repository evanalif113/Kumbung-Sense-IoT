#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "ExampleFunctions.h"
#include <ArduinoJson.h>
#include <WiFiManager.h>

#define API_KEY "AIzaSyD14wZkvP46yP3jQAwzUBOSh9kf8m-7vwg"
#define DATABASE_URL "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "esp32@gmail.com"
#define USER_PASSWORD "1234567"

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false; // change to true to use non blocking

WiFiManager wm; // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )


// Pin dan LED indicator
uint8_t ledPin = 2; // GPIO 2
#define TRIGGER_PIN 0 // GPIO 0 untuk trigger

void processData(AsyncResult &aResult);

WiFiClientSecure ssl_client, stream_ssl_client;

// Firebase components
FirebaseApp app;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client), streamClient(stream_ssl_client);
RealtimeDatabase Database;

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

// Timer variables for loop
unsigned long lastStatusTime = 0;
const unsigned long statusInterval = 30000; // 30 detik

// Declare outputs
const int output1 = 17;
const int output2 = 16;

// Variabel UID global dan flag
String uid;
bool streamIsReady = false; // Flag untuk memastikan stream hanya di-setup sekali

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
}

void connectWiFi() {
  
  // wm.resetSettings(); // wipe settings

  if(wm_nonblocking) wm.setConfigPortalBlocking(false);

  // add a custom input field
  int customFieldLength = 40;


  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
  
  // test custom html input type(checkbox)
  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
  
  // test custom html(radio)
  const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
  new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
  
  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);

  // custom menu via array or vector
  // 
  // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
  // const char* menu[] = {"wifi","info","param","sep","restart","exit"}; 
  // wm.setMenu(menu,6);
  std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  wm.setMenu(menu);

  // set dark theme
  wm.setClass("invert");


  //set static ip
  // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
  // wm.setShowStaticFields(true); // force show static ip fields
  // wm.setShowDnsFields(true);    // force show dns field always

  // wm.setConnectTimeout(20); // how long to try to connect for before continuing
  //wm.setConfigPortalTimeout(30); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  // wifi scan settings
  // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
  // wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
  // wm.setShowInfoErase(false);      // do not show erase button on info page
  // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons
  
  // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("KumbungSense-Actuactor","jamur123"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect or hit timeout");
    // ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
  }

}

void checkButton(){
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if( digitalRead(TRIGGER_PIN) == LOW ){
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);
      
      if (!wm.startConfigPortal("KumbungSense-Actuator","jamur123")) {
        Serial.println("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  }
}

void SetupFirebase() {
    // Inisialisasi Firebase dan mulai proses autentikasi.
    // Listener akan di-setup di dalam callback 'processData' setelah auth berhasil.
    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
}

void processData(AsyncResult &aResult){
    // Keluar jika tidak ada hasil (dipanggil dari loop utama)
    if (!aResult.isResult())
        return;

    // Menampilkan log event, debug, dan error
    if (aResult.isEvent()){
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

        // ---- LOGIKA BARU DIMULAI DI SINI ----
        // Cek apakah ini event dari authTask dan apakah sudah berhasil (ready, code 10)
        if (aResult.uid() == "authTask" && aResult.eventLog().code() == 10 /* ready */) {
            // Cek flag agar tidak menjalankan ini berulang kali
            if (!streamIsReady) {
                // Dapatkan UID HANYA SETELAH auth berhasil
                uid = app.getUid();
                Serial.print("Authentication successful! UID: ");
                Serial.println(uid);

                // Buat path database yang benar menggunakan UID yang sudah didapat
                String dbPath = "/" + uid + "/aktuator/data/";
                Serial.print("Setting up database stream for path: ");
                Serial.println(dbPath);

                // Setup stream listener ke path yang benar
                streamClient.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
                Database.get(streamClient, dbPath, processData, true /* SSE mode */, "streamTask");
                
                streamIsReady = true; // Set flag menjadi true
            }
        }
        // ---- LOGIKA BARU SELESAI ----
    }

    if (aResult.isDebug()){
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError()){
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    // Ketika menerima data dari database stream
    if (aResult.available()){
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream()) {
            Serial.println("----------------------------");
            Firebase.printf("task: %s\n", aResult.uid().c_str());
            Firebase.printf("event: %s\n", RTDB.event().c_str());
            Firebase.printf("path: %s\n", RTDB.dataPath().c_str());
            Firebase.printf("data: %s\n", RTDB.to<const char *>());
            Firebase.printf("type: %d\n", RTDB.type());

            // Tipe 6: JSON (biasanya diterima saat pertama kali stream dimulai)
            if (RTDB.type() == 6) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, RTDB.to<String>());
                if (error) {
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.c_str());
                    return;
                }
                // Iterasi JSON dan set status awal output
                for (JsonPair kv : doc.as<JsonObject>()) {
                    int gpioPin = atoi(kv.key().c_str());
                    bool state = kv.value().as<bool>();
                    digitalWrite(gpioPin, state ? HIGH : LOW);
                    Serial.printf("Initial state for GPIO %d set to %s\n", gpioPin, state ? "HIGH" : "LOW");
                }
            }

            // Tipe 4 (boolean) atau 1 (integer) untuk update individu
            if (RTDB.type() == 4 || RTDB.type() == 1){
                // Path-nya adalah "/16" atau "/17", jadi kita ambil nomor GPIO dari path
                int GPIO_number = RTDB.dataPath().substring(1).toInt();
                bool state = RTDB.to<bool>();
                digitalWrite(GPIO_number, state);
                Serial.printf("Updating GPIO %d to %s\n", GPIO_number, state ? "HIGH" : "LOW");
            }
        }
    }
}

void setup(){
    Serial.begin(115200);
    connectWiFi(); // Connect to Wi-Fi

    // Deklarasi pin output
    pinMode(output1, OUTPUT);
    pinMode(output2, OUTPUT);
    pinMode(ledPin, OUTPUT);

    digitalWrite(output1, HIGH); // Set awal output1 ke LOW
    digitalWrite(output2, HIGH); // Set awal output2 ke LOW

    set_ssl_client_insecure_and_buffer(ssl_client);
    set_ssl_client_insecure_and_buffer(stream_ssl_client);

    ssl_client.setHandshakeTimeout(5);
    stream_ssl_client.setHandshakeTimeout(5);

    // Panggil fungsi setup Firebase
    SetupFirebase();
}

void loop(){
    // Menjaga koneksi dan tugas asinkron Firebase tetap berjalan
    app.loop();
    if (wm_nonblocking) wm.process(); // Proses WiFiManager jika non-blocking
    checkButton(); // Cek tombol untuk reset pengaturan
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected! Attempting to reconnect...");
        connectWiFi();
    }

    // Cetak status secara berkala untuk menunjukkan program berjalan
    unsigned long currentTime = millis();
    if (currentTime - lastStatusTime >= statusInterval){
        lastStatusTime = currentTime;
        digitalWrite(ledPin, !digitalRead(ledPin)); // Kedipkan LED
        Serial.printf("Program running for %lu ms. UID: %s\n", currentTime, uid.c_str());
    }
}
