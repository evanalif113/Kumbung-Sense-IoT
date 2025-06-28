//#define USE_SQL
#define USE_FIREBASE
#define USE_BH1750
#define USE_SHT31
//#define USE_OLED
//#define USE_DEBUG
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <FirebaseClient.h>
#include <ArduinoJson.h>
#include "time.h"

#ifdef USE_SHT31
    #include <Adafruit_SHT31.h>
#endif

#ifdef USE_OLED
    #include <Adafruit_SSD1306.h>
#endif

#ifdef USE_BH1750
    #include <BH1750.h>
#endif

#ifdef USE_SQL
    #include <HTTPClient.h>
#endif

uint8_t id_sensor = 2;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
//#define INDI_PIN 5
#define MOISTURE_PIN 34
#define TRIGGER_PIN 0

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false; // change to true to use non blocking

WiFiManager wm; // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )

#ifdef USE_SQL
String deviceName = "ESP32_Sensor";
String ServerPath = "http://192.168.1.101:2518/api/data-sensor/send";
#endif

#define API_KEY "AIzaSyD14wZkvP46yP3jQAwzUBOSh9kf8m-7vwg"
#define DATABASE_URL "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "esp32@gmail.com"
#define USER_PASSWORD "1234567"

// Pin dan LED indicator
// uint8_t ledPin = 2; // GPIO 2 //

//define pin LED
const int ledHijau = 4;   // Ganti sesuai pin yang digunakan
const int ledMerah = 16;
const int ledKuning = 17;

// // Pin dan LED indicator
// uint8_t ledPin = 2; // GPIO 2

void processData(AsyncResult &aResult);

// Fungsi untuk mengatur status LED
void setLedStatus(bool hijau, bool kuning, bool merah) {
    digitalWrite(ledHijau, hijau ? HIGH : LOW);
    digitalWrite(ledMerah, merah ? HIGH : LOW);
    digitalWrite(ledKuning, kuning ? HIGH : LOW);
}

WiFiClientSecure ssl_client;

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;
RealtimeDatabase Database;

#ifdef USE_OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#endif

#ifdef USE_SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

#ifdef USE_BH1750
BH1750 light;
#endif

// Global variables to store latest sensor readings
float latestTemperature = 0;
float latestHumidity = 0;
float latestMoisture = 0; // ubah ke float agar bisa simpan persen
float latestLight = 0;

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

void initializeSensors() {
#ifdef USE_SHT31
    if (!sht31.begin(0x44)) {
        Serial.println("Could not find SHT31 sensor!");
        while (1);
    }
#endif

#ifdef USE_OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed!");
        while (1);
    }
#endif

#ifdef USE_BH1750
    if (!light.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("Could not find BH1750 sensor!");
        while (1);
    }
#endif

    pinMode(MOISTURE_PIN, INPUT);

#ifdef USE_OLED
    display.clearDisplay();
    display.display();
#endif
}

void updateSensor() {
    setLedStatus(true, true, false); // LED kuning nyala saat proses pembacaan sensor
#ifdef USE_SHT31
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    sht31.heater(false);
#else
    float temperature = 0;
    float humidity = 0;
#endif

#ifdef USE_BH1750
    float lux = light.readLightLevel();
#else
    float lux = 0;
#endif

    int moistureValue = analogRead(MOISTURE_PIN);
    float moisturePercent = 100.0 - ((moistureValue / 4095.0) * 100.0); // 0 = 100% (basah), 4095 = 0% (kering)

    if (!isnan(temperature) && !isnan(humidity) && !isnan(lux)) {
        latestTemperature = temperature;
        latestHumidity = humidity;
        latestMoisture = moisturePercent; // simpan dalam persen
        latestLight = lux;

        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" °C");
        Serial.print("Humidity: ");
        Serial.print(humidity); 
        Serial.println(" %");
        Serial.print("Light Level: ");
        Serial.print(lux);
        Serial.println(" lx");
        Serial.print("Soil Moisture: ");
        Serial.print(moisturePercent);
        Serial.println(" %");

#ifdef USE_OLED
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Sensor Data");
        display.print("Temp : ");
        display.print(temperature);
        display.write(167);
        display.println(" C");
        display.print("Humi : ");
        display.print(humidity);
        display.println(" Rh%");
        display.print("Light: ");
        display.print(lux);
        display.println(" lux");
        display.print("Moist: ");
        display.print(moisturePercent);
        display.println(" %");
        display.display();
#endif
    } else {
        Serial.println("Failed to read sensor!");
    }
    setLedStatus(true, false, false); // Matikan semua LED setelah pembacaan sensor
}

void sendDataToSQLServer() {
#ifdef USE_SQL
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Skipping data send.");
        return;
    }
    HTTPClient http;
    String url = ServerPath;
    url += "?id_sensor=" + String(id_sensor);
    url += "&temperature=" + String(latestTemperature, 2);
    url += "&humidity=" + String(latestHumidity, 2);
    url += "&moisture=" + String(latestMoisture);
    url += "&light=" + String(latestLight, 2);

    Serial.print("Request URL: ");
    Serial.println(url);

    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        if (httpResponseCode == 200) {
            Serial.println("Data berhasil dikirim ke server!");
        }
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        Serial.println("Possible reasons: server not running, wrong URL, firewall, or network issues.");
    }
    http.end();
#endif
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}



void SetupFirebase() {
    configTime(0, 0, "time.google.com", "pool.ntp.org"); // Initialize NTP Client
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl_client.setInsecure();

    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    Database.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");   
}

void SendDataToFirebase() {
  // Update NTP time
  setLedStatus(true, true, false); // LED hijau nyala saat proses kirim data
  unsigned long timestamp;
  timestamp = getTime();// Get current epoch time

  //JSON Constructor by ArduinoJSON
  JsonDocument docW;

  docW["temperature"] = latestTemperature;
  docW["humidity"] = latestHumidity;
  docW["light"] = latestLight;
  docW["moisture"] = latestMoisture;
  docW["timestamp"] = timestamp;

  String dataTani;

  docW.shrinkToFit();  // optional
  serializeJson(docW, dataTani);
   String uid = app.getUid();
   Serial.printf("Firebase UID: %s\n", uid.c_str());
  // Dynamically use timestamp in the path
  String dbPath = "/"+uid+"/sensor/data/" + timestamp;
  Database.set<object_t>(aClient, dbPath.c_str(), object_t(dataTani), processData, "setTask");
  setLedStatus(true, false, false); // LED kuning nyala saat proses selesai
}


void processData(AsyncResult &aResult) {
#ifdef USE_DEBUG
    // Exits when no result available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
#endif
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
  res = wm.autoConnect("KumbungSense-Sensor","jamur123"); // password protected ap

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
      delay(10000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);
      
      if (!wm.startConfigPortal("KumbungSense-Sensor","jamur123")) {
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

void setup() {
    Serial.begin(115200);
    // pinMode(ledPin, OUTPUT);
    // digitalWrite(ledPin, HIGH); // Matikan LED awalnya
    pinMode(ledHijau, OUTPUT);
    pinMode(ledMerah, OUTPUT);
    pinMode(ledKuning, OUTPUT);
    pinMode(TRIGGER_PIN, INPUT); // Set trigger pin as input with pull-up resistor
    setLedStatus(true, true, true); // Semua LED mati awalnya
    delay(5000); // Tunggu 1 detik untuk memastikan LED mati
    setLedStatus(false, false, false); // Semua LED mati setelah setup
    Wire.begin();
    connectWiFi();         // Gunakan WiFiManager
    setLedStatus(true, true, false); // Semua LED mati awalnya
    initializeSensors();
    setLedStatus(false, false, false); // Semua LED mati setelah setup
    setLedStatus(true, true, false); // Semua LED mati awalnya
    #ifdef USE_FIREBASE
    SetupFirebase();       // Inisialisasi Firebase
    setLedStatus(false, false, false); // Semua LED mati setelah setup
    #endif

}

static unsigned long previousMillis;
const unsigned long interval = 30000;


void loop() {
    app.loop();
    if(wm_nonblocking) wm.process();
    checkButton();
    if (WiFi.status() != WL_CONNECTED) {
        setLedStatus(true, false, true); // LED merah dan kuning nyala saat WiFi terputus
        Serial.println("WiFi disconnected! Attempting to reconnect...");
        connectWiFi();
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        updateSensor();
        #ifdef USE_SQL
        sendDataToSQLServer();
        #endif
        #ifdef USE_FIREBASE
        SendDataToFirebase(); // Kirim data ke Firebase
        #endif
        previousMillis = currentMillis;
    }
}


