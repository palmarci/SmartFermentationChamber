
// PINS
#define LED_PIN 2 // builtin
#define ONEWIRE_BUS_PIN 14
#define BME_I2C_SDA_PIN 32
#define BME_I2C_SCL_PIN 25
#define RELAY_PIN_HEATER 13
#define RELAY_PIN_HUMIDIFIER 27

// COMMUNICATION
#define BME_I2C_ID 0x76
#define ACCESS_POINT_IP "192.168.1.1"
#define FORCE_USE_HOTSPOT 0
#define WIFI_DEFAULT_SSID = "***REMOVED***"
#define WIFI_DEFAULT_PW = "***REMOVED***"
#define MQTT_DEFAULT_BROKER "192.168.1.100"
#define MQTT_DEFAULT_PORT 1883

// IDENTIFICATION
String HOSTNAME = "FermControl";
int VERSION = 2;
String DISPLAY_NAME = HOSTNAME + "v" + String(VERSION);


//#define UPDATE_DELTA 200 // in ms

//#define UI_USE_FS // dont use flash for ui, we hardly have enough as it is


// uncomment to disable verbose logging
//#define DEBUG_ESPUI
#define TELEGRAM_DEBUG

#define MEMORY_REPORTING_TIME 5 //seconds between displaying free ram

// #define TELEGRAM_BOT_TOKEN "***REMOVED***"
// #define TELEGRAM_CHAT_ID "***REMOVED***"
// #define TELEGRAM_WAITFOR 10000

#define INVALID_MINIMUM_TEMP 0
#define INVALID_MAX_TEMP 80

//#define SWITCH_DELAY 3 * 1000 //3 seconds