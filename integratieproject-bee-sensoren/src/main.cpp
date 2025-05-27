#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include "secrets.h"         // Zorg dat SSID, PASSWORD, en SERVER_URL hier correct zijn ingesteld

// DHT22 Setup
#define DHTPIN1 D13           // GPIO pin voor DHT22 sensor 1
#define DHTPIN2 D12         // GPIO pin voor DHT22 sensor 2
#define DHTTYPE DHT22
#define BUTTON_PIN 0  // GPIO 0 = ingebouwde knop (FireBeetle ESP32-S3)

DHT_Unified dht1(DHTPIN1, DHTTYPE);
DHT_Unified dht2(DHTPIN2, DHTTYPE);

// SHT4x Setup (I2C: SDA=GPIO8, SCL=GPIO9 op FireBeetle ESP32-S3)
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
bool sht4x_found = false; // Flag om bij te houden of SHT4x gevonden is in setup

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("INFO: Setup started!");
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Interne pull-up weerstand aanzetten
  dht1.begin();
  dht2.begin();
  Serial.println("INFO: DHT22 sensors initialized");

  if (!sht4.begin(&Wire)) {
    Serial.println("ERROR: Couldn't find SHT4x sensor! SHT4x data will be unavailable.");
    sht4x_found = false;
  } else {
    Serial.println("INFO: SHT4x initialized");
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    sht4x_found = true;
  }

  Serial.println("INFO: Attempting WiFi connection...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID);

  Serial.print("INFO: Connecting to WiFi");
  int wifi_connect_timeout_seconds = 30;
  int checks_per_second = 2;
  for (int i = 0; i < wifi_connect_timeout_seconds * checks_per_second; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    Serial.print(".");
    delay(1000 / checks_per_second);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nINFO: ✅ WiFi connected!");
    Serial.println("INFO: IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nERROR: ❌ WiFi connection FAILED after timeout!");
    Serial.print("ERROR: WiFi Status Code: ");
    Serial.println(WiFi.status());
  }

  Serial.println("INFO: Setup finished. Starting main loop...");
}

void loop() {
  Serial.println("--------------------");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WARNING: WiFi disconnected! Attempting to reconnect...");
    WiFi.disconnect();
    WiFi.reconnect();

    int reconnect_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && reconnect_attempts < 10) {
        delay(500);
        Serial.print("*");
        reconnect_attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nINFO: ✅ WiFi reconnected!");
    } else {
        Serial.println("\nERROR: ❌ WiFi reconnect FAILED! Skipping data send for this cycle.");
        delay(10000);
        return;
    }
  }

  // Read DHT22 Sensor 1
  sensors_event_t event;
  float dht1Temp = NAN, dht1Hum = NAN;
  dht1.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    dht1Temp = event.temperature;
  } else {
    Serial.println("ERROR: Failed to read temperature from DHT1");
  }
  dht1.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    dht1Hum = event.relative_humidity;
  } else {
    Serial.println("ERROR: Failed to read humidity from DHT1");
  }

  delay(2000); // Wait before reading second DHT22

  // Read DHT22 Sensor 2
  float dht2Temp = NAN, dht2Hum = NAN;
  dht2.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    dht2Temp = event.temperature;
  } else {
    Serial.println("ERROR: Failed to read temperature from DHT2");
  }
  dht2.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    dht2Hum = event.relative_humidity;
  } else {
    Serial.println("ERROR: Failed to read humidity from DHT2");
  }

  float shtTempVal = NAN, shtHumVal = NAN;
  if (sht4x_found) {
    sensors_event_t shtHumidityEvent, shtTempEvent;
    if (sht4.getEvent(&shtHumidityEvent, &shtTempEvent)) {
        shtTempVal = shtTempEvent.temperature;
        shtHumVal = shtHumidityEvent.relative_humidity;
    } else {
        Serial.println("ERROR: Failed to read data from SHT4x (sensor was found in setup)");
    }
  }

  StaticJsonDocument<384> doc;

  if (!isnan(dht1Temp)) doc["dht1_temp"] = round(dht1Temp * 10.0) / 10.0;
  if (!isnan(dht1Hum))  doc["dht1_hum"]  = round(dht1Hum * 10.0) / 10.0;
  if (!isnan(dht2Temp)) doc["dht2_temp"] = round(dht2Temp * 10.0) / 10.0;
  if (!isnan(dht2Hum))  doc["dht2_hum"]  = round(dht2Hum * 10.0) / 10.0;
  if (!isnan(shtTempVal)) doc["sht_temp"] = round(shtTempVal * 10.0) / 10.0;
  if (!isnan(shtHumVal))  doc["sht_hum"]  = round(shtHumVal * 10.0) / 10.0;

  String jsonDataForSerial;
  serializeJsonPretty(doc, jsonDataForSerial);
  Serial.println("INFO: Data to be sent to server:");
  Serial.println(jsonDataForSerial);

  if (WiFi.status() == WL_CONNECTED && doc.size() > 0) {
    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("INFO: Sending HTTP POST request to " + String(SERVER_URL));
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.print("INFO: HTTP Response code: ");
      Serial.println(httpResponseCode);
      String responsePayload = http.getString();
      Serial.println("INFO: Server response: " + responsePayload);
    } else {
      Serial.print("ERROR: HTTP POST failed, error: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else if (doc.size() == 0) {
    Serial.println("WARNING: No valid sensor data to send for this cycle.");
  } else {
    Serial.println("WARNING: WiFi not connected. Skipping HTTP POST.");
  }

  delay(60000);  // Wacht 60 seconden (1 minuut) voor de volgende cyclus
}
