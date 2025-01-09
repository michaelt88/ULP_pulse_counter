// Configuration.h
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define WIFI_SSID "mike"
//#define WIFI_SSID "mike2"
#define WIFI_PASSWORD "michaelt"
#define MQTT_SERVER "192.168.0.119"
#define MQTT_PORT 1883
#define MQTT_USER "esp"
#define MQTT_PASSWORD "esp"

#define TOPIC_PREFIX "home"


//#define DEVICE_NAME "esp32_meter"
//#define ULP_SLEEP_TIME 10
//#define ULP_COUNT_PIN_PULLUP_EN true

// Water meter config
//#define DEVICE_NAME "water"
//#define ULP_SLEEP_TIME 120
//#define ULP_COUNT_PIN_PULLUP_EN true

// Power meter config
#define DEVICE_NAME "power"
#define ULP_SLEEP_TIME 300
#define ULP_COUNT_PIN_PULLUP_EN false



#define GPIO_SENSOR_PIN GPIO_NUM_14  // GPIO pin connected to the sensor
#define RTC_GPIO_INDEX 16            // attain dynamically with: rtc_io_number_get(GPIO_SENSOR_PIN)
//https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html

#define LED_PIN 2


#define BATTERY_PIN 32        // ADC pin for battery voltage (use ADC1 pins 32-29 as wifi can interfere with ADC2 pins)
#define VOLTAGE_DIVIDER 2.31   // Voltage divider ratio (R1 + R2) / R2  - should be 2.0 - however may be some issue with the voltage reg etc
#define ADC_RESOLUTION 4095   // 12-bit ADC resolution
#define REF_VOLTAGE 3.3       // Reference voltage of ESP32 ADC



#include <Arduino.h> //for serial

void loadConfiguration();



#endif
