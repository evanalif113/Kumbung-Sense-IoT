#include <Arduino.h>

//Tiga parameter di bawah ini biasanya ada saat kita membuat template baru
#define BLYNK_TEMPLATE_ID "TMPL6fyxp5vwW"
#define BLYNK_TEMPLATE_NAME "Weather Station"
#define BLYNK_AUTH_TOKEN "57fuuTbIWci34nFvySlak4fhzS-932bi"

#define BLYNK_PRINT Serial //Kita menggunakan Blynk serial

#include <WiFi.h> //Library WiFi
#include <WiFiClient.h> //Library WiFiClient
#include <BlynkSimpleEsp32.h> //Library BlynkESP32

char ssid[] = "server"; //Nama WiFi yang digunakan
char pass[] = "jeris6467"; //Password WiFi yang digunakan

BlynkTimer timer; //Untuk push data dibutuhkan blynk timer (untuk code push data dapat dilihat di blynk example)

//SENSOR SHT31
#include <Wire.h>
#include "Adafruit_SHT31.h" // Library SHT31
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31(); // Buat objek sensor SHT31

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float suhu, kelembaban; // Variabel suhu dan kelembaban

void myTimerEvent()
{
  suhu = sht31.readTemperature();
  kelembaban = sht31.readHumidity();

  Serial.println("Suhu: " + String(suhu, 2) + "C");
  Serial.println("Kelembaban: " + String(kelembaban, 2) + "%");

  Blynk.virtualWrite(V1, suhu);
  Blynk.virtualWrite(V2, kelembaban);

  // Tampilkan ke OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Suhu:");
  display.setCursor(50, 0);
  display.print(suhu, 1);
  display.print(" C");

  display.setCursor(0, 20);
  display.print("Hum:");
  display.setCursor(50, 20);
  display.print(kelembaban, 1);
  display.print(" %");

  display.display();
}

void setup()
{
  Serial.begin(115200); //Menginisiasi serial monitor

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); //Menginisiasi Blynk

  if (!sht31.begin(0x44)) { // Alamat default SHT31
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1) delay(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Weather Station");
  display.display();

  timer.setInterval(1000L, myTimerEvent); //Mengirim data tiap satu detik
}


void loop()
{
  Blynk.run(); //Menjalankan Bylnk
  timer.run(); //Menjalankan timer
}