#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include "settings.c"

#define SEALEVELPRESSURE_HPA (1013.25)

ADC_MODE(ADC_VCC);

Adafruit_BME680 bme; // I2C

WiFiClient net;
PubSubClient client(net);

// Enter in deep bear sleep
void deepSleep () {
  Serial.println("Going deeep sleeep!");
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  // convert from seconds to microseconds
  ESP.deepSleep(DEEP_SLEEP_FOR * 1000000);
}

void setupWifi() {
  Serial.println("----------");
  Serial.println("Connecting to: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED){
    Serial.print(".");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    delay(500);
  }
  Serial.println("OK!");
  randomSeed(micros());
  printWifiInfo();
}

boolean ensureMqttConnection() {
  client.setServer(MQTT_HOST, MQTT_PORT);
  Serial.print("Attempting MQTT connection, ");
  // Create a random client ID
  String clientId = "EnvSensor-";
  clientId += String(random(0xffff), HEX);
  Serial.print("client id: ");
  Serial.println(clientId);
  if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("Connected!");
    return true;
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    return false;
  }
}

void printWifiInfo() {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

boolean setupSensor() {
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    return false;
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
  return bme.performReading();
}

void publish(char * topic, float value) {
  char tmp[50];
  String tmp_str = String(value);
  tmp_str.toCharArray(tmp, tmp_str.length() + 1);
  client.publish(topic, tmp, false);
}

int getBatteryLevel() {
  return ESP.getVcc();
}

void setup() {
  Serial.begin(115200);
  setupWifi();
  if (ensureMqttConnection()) {
    if (setupSensor()) {
      publish(MQTT_TEMPERATURE_TOPIC, bme.temperature);
      publish(MQTT_HUMIDITY_TOPIC, bme.humidity);
      publish(MQTT_PRESSURE_TOPIC, bme.pressure / 100.0);
      publish(MQTT_GAS_RESISTANCE_TOPIC, bme.gas_resistance / 1000.0);
      publish(MQTT_ALTITUDE_TOPIC, bme.readAltitude(SEALEVELPRESSURE_HPA));
      publish(MQTT_VOLTAGE_TOPIC, getBatteryLevel());
    }
    client.loop();
  }

  deepSleep();
}

void loop() {}
