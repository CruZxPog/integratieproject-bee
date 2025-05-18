#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include "secrets.h"
// Wi-Fi credentials


// DHT22 Setup
#define DHTPIN1 11
#define DHTPIN2 4 
#define DHTTYPE DHT22 
DHT_Unified dht1(DHTPIN1, DHTTYPE);
DHT_Unified dht2(DHTPIN2, DHTTYPE);

// SHT4x Setup
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing sensors...");

  // Initialize DHT22 sensors
  dht1.begin();
  dht2.begin();
  Serial.println("DHT22 sensors initialized");

  // Initialize SHT4x
  if (!sht4.begin(&Wire)) {
    Serial.println("Couldn't find SHT4x sensor");
    while (1) delay(1);
  }
  Serial.println("SHT4x initialized");

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  // Connect to Wi-Fi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void loop() {
  // Read DHT22 Sensor 1
  sensors_event_t event;
  dht1.temperature().getEvent(&event);
  float dht1Temp = event.temperature;
  dht1.humidity().getEvent(&event);
  float dht1Hum = event.relative_humidity;

  // Read DHT22 Sensor 2
  dht2.temperature().getEvent(&event);
  float dht2Temp = event.temperature;
  dht2.humidity().getEvent(&event);
  float dht2Hum = event.relative_humidity;

  // Read SHT4x
  sensors_event_t shtHumidity, shtTemp;
  sht4.getEvent(&shtHumidity, &shtTemp);

  // Format data like Serial Monitor
  String dataLine;
  dataLine += "DHT1 " + String(dht1Temp) + "°C, ";
  dataLine += "DHT1 " + String(dht1Hum) + "%, ";
  dataLine += "DHT2 " + String(dht2Temp) + "°C, ";
  dataLine += "DHT2 " + String(dht2Hum) + "%, ";
  dataLine += "SHT " + String(shtTemp.temperature) + "°C, ";
  dataLine += "SHT " + String(shtHumidity.relative_humidity) + "%";

  // Print to Serial
  Serial.println(dataLine);

  // Send to Flask
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<256> doc;
    doc["data"] = dataLine;

    String json;
    serializeJson(doc, json);

    int code = http.POST(json);
    Serial.print("HTTP Response: ");
    Serial.println(code);
    String response = http.getString();
    Serial.println("Server says: " + response);

    http.end();
  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(3000);  // Wait 3 seconds before next reading
}
