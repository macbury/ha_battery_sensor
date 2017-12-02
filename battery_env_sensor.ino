#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

#include "settings.c"

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

Adafruit_BMP280 bme;
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

void setupSensor() {
  if (!bme.begin()) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
}

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupSensor();
  if (ensureMqttConnection()) {

    client.publish(MQTT_TEMPERATURE_TOPIC, "0");
    client.loop();
  }

  deepSleep();
}

void loop() {}
