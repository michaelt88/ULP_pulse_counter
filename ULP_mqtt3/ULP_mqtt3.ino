// Refactored ESP32 Low Power Power/Water Meter Code

// Main sketch file
#include <WiFiHandler.h>
#include <ULPHandler.h>
#include <Configuration.h>

void setup() {
    
    pinMode(BATTERY_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // turn on during init
    Serial.begin(115200);

    // Load configuration
    loadConfiguration();

    if (!ULPHandler::isULPInitialized()) {
        // Setup ULP if not already initialized
        ULPHandler::setupULP();
    }
    else {
        Serial.println("ULP program already initialized.");
    }

    // Retrieve count and rate from ULP
    uint16_t count = ULPHandler::getCount();
    float rate = ULPHandler::getRate();

    // Connect to WiFi and send data
    if (WiFiHandler::setupWiFi() == false) {
      Serial.println("Couldn't connect to wifi - sleeping");
      ULPHandler::enter_deep_sleep();
    }

    delay(100);
    
    if (WiFiHandler::setupMQTT() == false) {
      Serial.println("Couldn't connect to wifi - sleeping");
      ULPHandler::enter_deep_sleep();
    }

    delay(100);
    
    WiFiHandler::sendData(count, rate);

    WiFiHandler::disconnectMQTT();
    WiFiHandler::disconnectWiFi();

    // Enter deep sleep
    digitalWrite(LED_PIN, LOW);  // turn off during sleep
    ULPHandler::enter_deep_sleep();
}

void loop() {
    // Empty loop as everything is handled in setup and deep sleep
}
