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

boolean is_night = false;

void on_mqtt_message(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  #ifdef LOGGING
    Serial.print("Current sun state is:");
    Serial.println(message);
  #endif

  if (String(message) == "below_horizon") {
    #ifdef LOGGING
      Serial.println("Below horizon!");
    #endif
    is_night = true;
  } else {
    #ifdef LOGGING
      Serial.println("Above horizon!");
    #endif
    is_night = false;
  }
}

// Enter in deep bear sleep
void deepSleep () {
  #ifdef LOGGING
    Serial.println("Going deeep sleeep!");
  #endif
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  int sleep_time = DEEP_SLEEP_FOR_DAY;
  
  if (getBatteryLevel() < BATTERY_SAVING_VOLTAGE) {
    sleep_time = DEEP_POWER_SAVE_SLEEP;
  } else if (is_night) {
    sleep_time = DEEP_SLEEP_FOR_NIGHT;
  }

  ESP.deepSleep(sleep_time * 1000000);
}

void setupWifi() {
  #ifdef LOGGING
    Serial.println("----------");
    Serial.println("Connecting to: ");
    Serial.println(WIFI_SSID);
  #endif
  WiFi.mode(WIFI_STA);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED){
    #ifdef LOGGING
      Serial.print(".");
    #endif
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    delay(500);
  }
  #ifdef LOGGING
    Serial.println("OK!");
    randomSeed(micros());
    printWifiInfo();
  #endif
}

boolean ensureMqttConnection() {
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(on_mqtt_message);
  String clientId = "EnvSensor-";
  clientId += String(random(0xffff), HEX);
  #ifdef LOGGING
    Serial.print("Attempting MQTT connection, ");
    Serial.print("client id: ");
    Serial.println(clientId);
  #endif
  if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    #ifdef LOGGING
      Serial.println("Connected!");
    #endif
    client.subscribe("home/sun");
    return true;
  } else {
    #ifdef LOGGING
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    #endif
    // Wait 5 seconds before retrying
    return false;
  }
}

void printWifiInfo() {
  #ifdef LOGGING
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}

boolean setupSensor() {
  if (!bme.begin()) {
    #ifdef LOGGING
      Serial.println("Could not find a valid BME680 sensor, check wiring!");
    #endif
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
  #ifdef LOGGING
    Serial.begin(115200);
  #endif
  setupWifi();
  if (ensureMqttConnection()) {
    client.loop();
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
