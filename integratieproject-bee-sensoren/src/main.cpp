#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>

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

  // Initialize first DHT22
  dht1.begin();
  Serial.println("DHT22 Sensor 1 initialized");

  // Initialize second DHT22
  dht2.begin();
  Serial.println("DHT22 Sensor 2 initialized");

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
  // DHT22 Sensor 1
  sensors_event_t dhtEvent;
  dht1.temperature().getEvent(&dhtEvent);
  float dht1Temp = dhtEvent.temperature;
  dht1.humidity().getEvent(&dhtEvent);
  float dht1Hum = dhtEvent.relative_humidity;

  // DHT22 Sensor 2
  dht2.temperature().getEvent(&dhtEvent);
  float dht2Temp = dhtEvent.temperature;
  dht2.humidity().getEvent(&dhtEvent);
  float dht2Hum = dhtEvent.relative_humidity;

  // SHT4x
  sensors_event_t shtHumidity, shtTemp;
  uint32_t timestamp = millis();
  sht4.getEvent(&shtHumidity, &shtTemp);
  timestamp = millis() - timestamp;

  // Print alles op één regel, gescheiden door komma's
  Serial.print("DHT1 "); Serial.print(dht1Temp); Serial.print("°C, ");
  Serial.print("DHT1 "); Serial.print(dht1Hum); Serial.print("%, ");
  Serial.print("DHT2 "); Serial.print(dht2Temp); Serial.print("°C, ");
  Serial.print("DHT2 "); Serial.print(dht2Hum); Serial.print("%, ");
  Serial.print("SHT "); Serial.print(shtTemp.temperature); Serial.print("°C, ");
  Serial.print("SHT "); Serial.print(shtHumidity.relative_humidity); Serial.print("%, ");
  delay(3000);
}
