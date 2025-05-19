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
#define DHTPIN1 11           // GPIO pin voor DHT22 sensor 1
#define DHTPIN2 4            // GPIO pin voor DHT22 sensor 2
#define DHTTYPE DHT22
DHT_Unified dht1(DHTPIN1, DHTTYPE);
DHT_Unified dht2(DHTPIN2, DHTTYPE);

// SHT4x Setup (I2C: SDA=GPIO8, SCL=GPIO9 op FireBeetle ESP32-S3)
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
bool sht4x_found = false; // Flag om bij te houden of SHT4x gevonden is in setup

void setup() {
  Serial.begin(115200);
  delay(1000); // Geef de seriële monitor even de tijd om te stabiliseren
  Serial.println("INFO: Setup started!");

  // Initialize DHT22 sensors
  dht1.begin();
  dht2.begin();
  Serial.println("INFO: DHT22 sensors initialized");

  // Initialize SHT4x
  if (!sht4.begin(&Wire)) { // Wire wordt impliciet geïnitialiseerd door de library indien nodig
    Serial.println("ERROR: Couldn't find SHT4x sensor! SHT4x data will be unavailable.");
    sht4x_found = false;
  } else {
    Serial.println("INFO: SHT4x initialized");
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER); // Meestal wil je de heater uit laten tenzij je condensatieproblemen hebt
    sht4x_found = true;
  }

  // Connect to Wi-Fi
  Serial.println("INFO: Attempting WiFi connection...");
  WiFi.mode(WIFI_STA); // Zet de ESP32 in Station mode (client)
  WiFi.begin(SSID); // Haalt credentials uit secrets.h

  Serial.print("INFO: Connecting to WiFi");
  int wifi_connect_timeout_seconds = 30; // Maximaal 30 seconden wachten op verbinding
  int checks_per_second = 2;
  for (int i = 0; i < wifi_connect_timeout_seconds * checks_per_second; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      break; // Verbinding gelukt
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
    // In een productieomgeving zou je hier misschien een fallback of retry mechanisme willen
  }

  Serial.println("INFO: Setup finished. Starting main loop...");
}

void loop() {
  Serial.println("--------------------"); // Scheidingsteken voor leesbaarheid in monitor

  // Controleer WiFi verbinding aan het begin van elke loop iteratie
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WARNING: WiFi disconnected! Attempting to reconnect...");
    WiFi.disconnect(); // Optioneel, maar kan helpen bij een schone reconnect
    WiFi.reconnect();  // Ingebouwde reconnect functie

    int reconnect_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && reconnect_attempts < 10) { // Probeer ~5 seconden lang
        delay(500);
        Serial.print("*"); // Ander teken voor reconnect poging
        reconnect_attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nINFO: ✅ WiFi reconnected!");
    } else {
        Serial.println("\nERROR: ❌ WiFi reconnect FAILED! Skipping data send for this cycle.");
        delay(10000); // Wacht langer voordat je de loop opnieuw probeert
        return;       // Sla de rest van deze loop iteratie over
    }
  }

  // Read DHT22 Sensor 1
  sensors_event_t event;
  float dht1Temp = NAN, dht1Hum = NAN; // Initialiseer met Not-a-Number
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

  // Read SHT4x
  float shtTempVal = NAN, shtHumVal = NAN;
  if (sht4x_found) { // Alleen lezen als de sensor in setup is gevonden
    sensors_event_t shtHumidityEvent, shtTempEvent;
    if (sht4.getEvent(&shtHumidityEvent, &shtTempEvent)) { // getEvent retourneert true bij succes
        shtTempVal = shtTempEvent.temperature;
        shtHumVal = shtHumidityEvent.relative_humidity;
    } else {
        Serial.println("ERROR: Failed to read data from SHT4x (sensor was found in setup)");
    }
  } else {
    // Optioneel: Serial.println("INFO: SHT4x sensor not available. Skipping SHT4x reading.");
  }

  // Voorbereiden JSON data
  StaticJsonDocument<384> doc; // Capaciteit voor de JSON data
                               // De deprecation warning kan hier nog steeds optreden, maar is functioneel OK.

  // Voeg alleen geldige (niet NAN) sensorwaarden toe aan het JSON document
  if (!isnan(dht1Temp)) doc["dht1_temp"] = round(dht1Temp * 10.0) / 10.0; // Afronden op 1 decimaal
  if (!isnan(dht1Hum))  doc["dht1_hum"]  = round(dht1Hum * 10.0) / 10.0;
  if (!isnan(dht2Temp)) doc["dht2_temp"] = round(dht2Temp * 10.0) / 10.0;
  if (!isnan(dht2Hum))  doc["dht2_hum"]  = round(dht2Hum * 10.0) / 10.0;
  if (!isnan(shtTempVal)) doc["sht_temp"] = round(shtTempVal * 10.0) / 10.0;
  if (!isnan(shtHumVal))  doc["sht_hum"]  = round(shtHumVal * 10.0) / 10.0;
  
  // Optioneel: voeg een timestamp toe als je die wilt (bijv. van NTP of millis())
  // doc["timestamp_ms"] = millis();

  // Print de data die verstuurd gaat worden voor debugging
  String jsonDataForSerial;
  serializeJsonPretty(doc, jsonDataForSerial); // 'Pretty' voor betere leesbaarheid in monitor
  Serial.println("INFO: Data to be sent to server:");
  Serial.println(jsonDataForSerial);

  // Verstuur data naar Flask server
  if (WiFi.status() == WL_CONNECTED && doc.size() > 0) { // Alleen versturen als verbonden en er data is
    HTTPClient http;
    http.begin(SERVER_URL); // SERVER_URL uit secrets.h
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000); // Timeout van 10 seconden voor de HTTP request

    String jsonPayload;
    serializeJson(doc, jsonPayload); // Serialize voor versturen (niet 'pretty')

    Serial.println("INFO: Sending HTTP POST request to " + String(SERVER_URL));
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.print("INFO: HTTP Response code: ");
      Serial.println(httpResponseCode);
      String responsePayload = http.getString(); // Lees het antwoord van de server
      Serial.println("INFO: Server response: " + responsePayload);
    } else {
      Serial.print("ERROR: HTTP POST failed, error: ");
      Serial.println(http.errorToString(httpResponseCode).c_str()); // Geeft meer info over HTTP fout
    }
    http.end(); // Sluit de HTTP client connectie
  } else if (doc.size() == 0) {
    Serial.println("WARNING: No valid sensor data to send for this cycle.");
  } else {
    // Dit geval zou al afgevangen moeten zijn door de WiFi check aan het begin van de loop
    Serial.println("WARNING: WiFi not connected. Skipping HTTP POST.");
  }

  delay(15000);  // Wacht 15 seconden (pas aan naar gewenst interval)
}