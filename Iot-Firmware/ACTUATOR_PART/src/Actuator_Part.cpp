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

// Pin dan LED indicator
uint8_t ledPin = 2; // GPIO 2

void processData(AsyncResult &aResult);

SSL_CLIENT ssl_client, stream_ssl_client;

// Firebase components
FirebaseApp app;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client), streamClient(stream_ssl_client);
RealtimeDatabase Database;

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

// Timer variables for loop
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds

// Declare outputs
const int output1 = 17;
const int output2 = 16;

void connectWiFi() {
    WiFiManager wm;
    // Jika sudah pernah connect, akan otomatis connect
    // Jika belum, ESP32 akan membuat AP captive portal untuk konfigurasi WiFi
    bool res = wm.autoConnect("ESP32-Sensing-Setup", "admin123"); // SSID dan password AP sementara
    if(!res) {
        Serial.println("Gagal connect WiFi, restart ESP...");
        delay(3000);
        ESP.restart();
    } else {
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

void initFirebase() {
    // Initialize Firebase
    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    // Set a database listener
    streamClient.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
    String uid = app.getUid();
    Serial.printf("Firebase UID: %s\n", uid.c_str());
    String dbPath = "/"+uid+"/aktuator/data/";
    Database.get(streamClient, dbPath, processData, true /* SSE mode (HTTP Streaming) */, "streamTask");
}

void processData(AsyncResult &aResult){
  // Exits when no result available when calling from the loop.
  if (!aResult.isResult())
    return;

  if (aResult.isEvent()){
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
  }

  if (aResult.isDebug()){
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()){
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  // When it receives data from the database
  if (aResult.available()){
    RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
    // we received data from the streaming client
    if (RTDB.isStream()) {
      Serial.println("----------------------------");
      Firebase.printf("task: %s\n", aResult.uid().c_str());
      Firebase.printf("event: %s\n", RTDB.event().c_str());
      Firebase.printf("path: %s\n", RTDB.dataPath().c_str());
      Firebase.printf("etag: %s\n", RTDB.ETag().c_str());
      Firebase.printf("data: %s\n", RTDB.to<const char *>());
      Firebase.printf("type: %d\n", RTDB.type());

      // RTDB.type = 6 means the result is a JSON : https://github.com/mobizt/FirebaseClient/blob/main/resources/docs/realtime_database_result.md#--realtime_database_data_type-type
      // You receive a JSON when you initialize the stream
      if (RTDB.type() == 6) {
        Serial.println(RTDB.to<String>());
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, RTDB.to<String>());
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }
        // Iterate through JSON object
        for (JsonPair kv : doc.as<JsonObject>()) {
          int gpioPin = atoi(kv.key().c_str()); // Convert key (e.g., "12") to int
          bool state = kv.value().as<bool>();
          digitalWrite(gpioPin, state ? HIGH : LOW);
        }
      }

      // RTDB.type() = 4 means the result is a boolean
      // RTDB.type() = 1 means the result is an integer
      // learn more here: https://github.com/mobizt/FirebaseClient/blob/main/resources/docs/realtime_database_result.md#--realtime_database_data_type-type
      if (RTDB.type() == 4 || RTDB.type() == 1){
        // get the GPIO number
        int GPIO_number = RTDB.dataPath().substring(1).toInt();
        bool state = RTDB.to<bool>();
        digitalWrite(GPIO_number, state);
        Serial.println("Updating GPIO State");
      }

      // The stream event from RealtimeDatabaseResult can be converted to values as following.
      /*bool v1 = RTDB.to<bool>();
      int v2 = RTDB.to<int>();
      float v3 = RTDB.to<float>();
      double v4 = RTDB.to<double>();
      String v5 = RTDB.to<String>();
      Serial.println(v5); */
    }
    else{
        Serial.println("----------------------------");
        Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
  }
}

void setup(){
    Serial.begin(115200);
    connectWiFi(); // Connect to Wi-Fi
    // Declare pins as outputs
    pinMode(output1, OUTPUT);
    pinMode(output2, OUTPUT);

    set_ssl_client_insecure_and_buffer(ssl_client);
    set_ssl_client_insecure_and_buffer(stream_ssl_client);

    ssl_client.setHandshakeTimeout(5);
    stream_ssl_client.setHandshakeTimeout(5);
    initFirebase();
}

void loop(){
  // Maintain authentication and async tasks
  app.loop();

  // Check if authentication is ready
  if (app.ready()){
    //Do nothing - everything works with callback functions
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      // Update the last send time
      lastSendTime = currentTime;
      Serial.printf("Program running for %lu\n", currentTime);        
    }
  }
}
