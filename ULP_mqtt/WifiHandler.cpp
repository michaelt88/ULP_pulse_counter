
// WiFiHandler.cpp
#include "WiFiHandler.h"
#include "Configuration.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool WiFiHandler::setupWiFi() {
  
  delay(10);

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  Serial.print("...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 10) {
    delay(1000);
    Serial.print(".");
    i++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
    return true;
  }
  else {
    return false;
  }
    
    
}

bool WiFiHandler::setupMQTT() {  
  // Set MQTT server
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  // Loop until we're reconnected
  int i = 0;
  while (!mqttClient.connected() && i < 10) {
    Serial.print("Attempting MQTT connection...");
    // Try to connect
    String deviceName = String(DEVICE_NAME) + String(random(1000, 9999));
    if (mqttClient.connect(deviceName.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    //if (mqttClient.connect(DEVICE_NAME, MQTT_USER, MQTT_PASSWORD)) {
    //if (client.connect("ESP32Client", mqtt_user, mqtt_password, "homeassistant/status", 0, true, "offline")) {
      Serial.println("Connected to MQTT broker");
      return true;
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      mqttClient.loop();
      Serial.println(" trying again in 5 seconds");
      delay(5000);
      i++;
    }
  }
  return false;
    
}

float readBatteryVoltage() {
    int rawADC = analogRead(BATTERY_PIN);
    Serial.println(rawADC);
    float voltage = (rawADC / float(ADC_RESOLUTION)) * REF_VOLTAGE * VOLTAGE_DIVIDER;
    return voltage;
}


void WiFiHandler::sendData(uint32_t count, float rate) {
  char countTopic[50], rateTopic[50], signalTopic[50], voltageTopic[50];
  snprintf(countTopic, sizeof(countTopic), "%s/%s/count", TOPIC_PREFIX, DEVICE_NAME);
  snprintf(rateTopic, sizeof(rateTopic), "%s/%s/rate", TOPIC_PREFIX, DEVICE_NAME);
  snprintf(signalTopic, sizeof(signalTopic), "%s/%s/signal", TOPIC_PREFIX, DEVICE_NAME);
  snprintf(voltageTopic, sizeof(voltageTopic), "%s/%s/voltage", TOPIC_PREFIX, DEVICE_NAME);


  mqttClient.publish(countTopic, String(count).c_str(), true);
  mqttClient.loop();
  mqttClient.publish(rateTopic, String(rate).c_str(), true);
  mqttClient.loop();

  // Get WiFi signal strength
  int32_t signalStrength = WiFi.RSSI();
  mqttClient.publish(signalTopic, String(signalStrength).c_str(), true);
  mqttClient.loop();

  // Send battery voltage
  float batteryVoltage = readBatteryVoltage();
  mqttClient.publish(voltageTopic, String(batteryVoltage).c_str(), true);
  mqttClient.loop();

  Serial.println(countTopic);
  Serial.println(rateTopic);
  Serial.println(signalTopic);
  Serial.println(voltageTopic);
  Serial.printf("Data sent: count=%u, rate=%.2f, signal=%d dBm, voltage=%.2f V\n", count, rate, signalStrength, batteryVoltage);
}

void WiFiHandler::disconnectMQTT() {
  mqttClient.loop();
  mqttClient.disconnect();
  delay(100);
  mqttClient.loop();
  delay(100);
}

void WiFiHandler::disconnectWiFi() {
  WiFi.disconnect(true); 
}
