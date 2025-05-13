#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>

// DHT22 Setup
#define DHTPIN 2
#define DHTTYPE DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);

// SHT4x Setup
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing sensors...");

  // Initialize DHT22
  dht.begin();
  Serial.println("DHT22 initialized");

  // Initialize SHT4x
  if (!sht4.begin(&Wire)) {
    Serial.println("Couldn't find SHT4x sensor");
    while (1) delay(1);
  }
  Serial.println("SHT4x initialized");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // Configure SHT4x
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
}

void loop() {
  Serial.println("---- Sensor Readings ----");

  // Read DHT22 Temperature and Humidity
  sensors_event_t dhtEvent;
  dht.temperature().getEvent(&dhtEvent);
  if (isnan(dhtEvent.temperature)) {
    Serial.println("Error reading DHT22 temperature!");
  } else {
    Serial.print("DHT22 Temperature: ");
    Serial.print(dhtEvent.temperature);
    Serial.println(" °C");
  }

  dht.humidity().getEvent(&dhtEvent);
  if (isnan(dhtEvent.relative_humidity)) {
    Serial.println("Error reading DHT22 humidity!");
  } else {
    Serial.print("DHT22 Humidity: ");
    Serial.print(dhtEvent.relative_humidity);
    Serial.println(" %");
  }

  // Read SHT4x Temperature and Humidity
  sensors_event_t shtHumidity, shtTemp;
  uint32_t timestamp = millis();
  sht4.getEvent(&shtHumidity, &shtTemp);
  timestamp = millis() - timestamp;

  Serial.print("SHT4x Temperature: ");
  Serial.print(shtTemp.temperature);
  Serial.println(" °C");

  Serial.print("SHT4x Humidity: ");
  Serial.print(shtHumidity.relative_humidity);
  Serial.println(" %");

  Serial.print("SHT4x Read Time: ");
  Serial.print(timestamp);
  Serial.println(" ms");

  Serial.println("--------------------------\n");
  delay(3000);
}
