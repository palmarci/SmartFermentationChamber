#ifndef CONFIG
#define CONFIG

#include <Arduino.h>
#include "secrets.h"

// ******** PINS ********
// see https://randomnerdtutorials.com/esp32-pinout-reference-gpios
#define LED_PIN 2 // builtin

#define ONEWIRE_BUS_PIN 4 // for the Dallas DS18B20

#define I2C_SDA_PIN 25 // for the BME280
#define I2C_SCL 14

#define PIN_HEATER 32
#define PIN_HEATER_INVERT false

#define PIN_HUMIDIFIER 23
#define PIN_HUMIDIFIER_INVERT false
//#define HUMIDIFER_PUSH_GATE 23

// ******** COMMUNICATION ********
#define SERIAL_SPEED 115200
#define BME_I2C_ID 0x76
#define WIFI_AP_DEFAULT_IP "192.168.1.1"
#define WIFI_CONNECT_TIMEOUT 15
#define WIFI_AP_MODE_FORCE 0

#define MQTT_DEFAULT_IP "192.168.1.12"
#define MQTT_DEFAULT_PORT 1883
#define MQTT_LOG_TOPIC "log"
#define MQTT_MEASUREMENT_TOPIC "measurement"
#define MQTT_HEARTBEAT_TOPIC "heartbeat"

// ******** NVM KEY VALUES ********
// THESE MUST BE SMALLER THAN 15 CHARS !!!
#define NVM_WIFI_SSID "WIFI_SSID"
#define NVM_WIFI_PW "WIFI_PW"
#define NVM_MQTT_IP "MQTT_IP"
#define NVM_MQTT_PORT "MQTT_PORT"
#define NVM_TARGET_TEMP "NVM_T_TEMP"
#define NVM_TARGET_HUM_ABS "NVM_T_HUM_ABS"
#define NVM_TARGET_HUM_REL "NVM_T_HUM_REL"
#define NVM_KEY "SFCH"
#define NVM_PID_KP "NVM_PID_KP"
#define NVM_PID_KI "NVM_PID_KI"
#define NVM_PID_KD "NVM_PID_KD"

// ******** SENSORS ********
#define INVALID_MINIMUM_TEMP 0
#define INVALID_MAX_TEMP 80
#define DEFAULT_TARGET_HUMIDITY_REL 60
#define DEFAULT_TARGET_HUMIDITY_ABS 30.71
#define DEFAULT_TARGET_TEMP 40
#define BME_HUMIDITY_OFFSET 0.0 // these  can be negative
#define BME_TEMP_OFFSET 0
#define DALLAS_TEMP_OFFSET 0.8
#define SENSOR_MEASURE_PERIOD 5 // seconds
#define SENSOR_MEASUREMENT_TIMEOUT (SENSOR_MEASURE_PERIOD * 4)
#define PID_ENABLED true
#define PID_DEFAULT_KP 0.5f
#define PID_DEFAULT_KI 50.f
#define PID_DEFAULT_KD 10.f

// ******** SYSTEM ********
#define HOSTNAME "SmartFermentationChamber"
#define MAX_TASK_HANDLES 10
#define RESTART_AFTER 24 // hours
#define AUTOPILOT_ENABLED_AT_STARTUP true 
#define DISABLE_BROWNOUT false // if usb port voltage dips below 5V
#define STACK_SIZE 20000
#define RELAY_PROTECTION_DELAY 3 // mins

// ******** MACROS ********
#define LOG_DEBUG 1
#define LOG_WARNING 2
#define LOG_PANIC 3
#define VERSION ("Build: " + String(__DATE__) + " " + String(__TIME__) + ", PID " + (PID_ENABLED ? "ON" : "OFF"))
#endif /* CONFIG */