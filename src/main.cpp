#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <ESPUI.h>
#include <esp_heap_caps.h>

#include "logic.h"
#include "config.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

WiFiClientSecure wifi_client_secure;
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
UniversalTelegramBot telegram_bot(TELEGRAM_BOT_TOKEN, wifi_client_secure);

// globals
OneWire onewire_bus(ONEWIRE_BUS_PIN);
DallasTemperature dallas_sensors(&onewire_bus);
Adafruit_BME280 bme_sensor;

float target_humidity = 60;
float target_temp = 40;
bool heater_enabled = false;
bool humidifier_enabled = false;
bool automatic_mode = true;
int target_temp_min = 25;
int target_temp_max = 70;
int hum_min = 40;
int hum_max = 100;

// runtime
bool lastmode = automatic_mode;
bool startup = true;
unsigned long memory_job_last = 0;
static long unsigned last_switch = 0;

// functions
void logprint(String text)
{
	String data = ("[" + String(millis()) + "] " + text);
	// TODO add remote logging
	Serial.println(data.c_str());
}

void halt(String error_text, int time_delay)
{
	logprint("ERROR: " + error_text + ", halting...");
	digitalWrite(RELAY_PIN_HEATER, LOW);
	digitalWrite(RELAY_PIN_HUMIDIFIER, LOW);
	while (true)
	{
		do_blink(time_delay);
	}
}

String bool_to_str(bool val)
{
	if (val)
	{
		return String("1");
	}
	else
	{
		return String("0");
	}
}

void send_mqtt_msg(String topic, String msg)
{
	if (!mqtt_client.connected())
	{
		logprint("[mqtt] not connected, connecting...");
		mqtt_client.connect(project_display_name);
	} 

	if (!mqtt_client.connected())
	{
		logprint("[mqtt] could not connect to mqtt, send failed");
		return;
	} 
	logprint("[mqtt] sending data: " + topic + " -> " + msg);
	mqtt_client.publish(topic.c_str(), msg.c_str());
	
}

void report_free_memory()
{
	uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
	uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
	float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
	logprint("[memory] " + String(percentageHeapFree) + " % free of " + String(totalHeapBytes / 1000) + "k");
	return;
}

void setup_logic()
{
	// setup uart & pins
	Serial.begin(115200);
	delay(1000);
	pinMode(LED_PIN, OUTPUT);
	pinMode(RELAY_PIN_HEATER, OUTPUT);
	pinMode(RELAY_PIN_HUMIDIFIER, OUTPUT);
	digitalWrite(RELAY_PIN_HEATER, LOW);
	digitalWrite(RELAY_PIN_HUMIDIFIER, LOW);
	// logprint("STARTING VERSION v" + String(VERSION));

	// setup dallas sensor
	dallas_sensors.begin();
	if (dallas_sensors.getDeviceCount() < 1)
	{
		halt("no dallas devices found on bus", 500);
	}
	dallas_sensors.setResolution(12);

	// setup bme sensor
	Wire.begin(BME_I2C_SDA_PIN, BME_I2C_SCL_PIN); // set i2c pins
	if (!bme_sensor.begin(BME_I2C_ID, &Wire))
	{
		halt("no BME280 sensor found", 1000);
	}

	// wifi_client_secure.setInsecure();
	mqtt_client.setServer(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT);

	// setup telegram bot
	wifi_client_secure.setCACert(TELEGRAM_CERTIFICATE_ROOT);
	telegram_bot.waitForResponse = TELEGRAM_WAITFOR;
}

bool check_temp_range_valid(float input)
{
	if (input < INVALID_MINIMUM_TEMP || input > INVALID_MAX_TEMP)
	{
		return false;
	}
	return true;
}

float get_humidity()
{
	float hum = bme_sensor.readHumidity();
#ifdef DEBUG_LOG_VERBOSE
	logprint("[bme] hum=" + String(hum));
#endif
	send_mqtt_msg(String(project_display_name) + "/measurements/humidity", String(hum));
	return hum;
}

void bme_check_lifecycle()
{
	sensors_event_t last_event;
	bme_sensor.getTemperatureSensor()->getEvent(&last_event);
	if (last_event.data[0] > 160 || isnan(last_event.data[0]))
	{ // TODO other invalid ranges?
		alert("bme sensor may be disconnected or faulty, rebooting...", true);
	}
}

float get_temp_bme()
{
	bme_check_lifecycle();

	float temp = bme_sensor.readTemperature();
#ifdef DEBUG_LOG_VERBOSE
	logprint("[bme] temp=" + String(temp));
#endif

	if (!check_temp_range_valid(temp))
	{
		alert("bme sensor read invalid value: " + String(temp) + ", halting...");
		halt("", 100);
	}

	return temp;
}

float get_temp_dallas()
{
	dallas_sensors.requestTemperatures();
	float temp = dallas_sensors.getTempCByIndex(0);
#ifdef DEBUG_LOG_VERBOSE
	logprint("[dallas] read temperature " + String(temp));
#endif
	if (temp < -126)
	{
		alert("dallas sensor may be disconnected or faulty, rebooting...", true);
		// logprint();
		//	reboot();
	}

	if (!check_temp_range_valid(temp))
	{
		alert("dallas sensor read invalid value: " + String(temp) + ", halting...");
		halt("", 100);
	}

	return temp;
}

void (*resetFunc)(void) = 0;
void reboot()
{
	resetFunc();
}

void do_blink(int time_delay)
{
	digitalWrite(LED_PIN, HIGH);
	delay(time_delay);
	digitalWrite(LED_PIN, LOW);
	delay(time_delay);
}

String get_status_text()
{
	String status_text = "Temperature inside the chamber: " + String(get_temp_bme()) + "\n" +
						 "Temperature inside the food: " + String(get_temp_dallas()) + "\n" +
						 "Humdity: " + String(get_humidity()) + "%";
	return status_text;
}


void do_logic_step()
{
	// handle startup
	if (startup)
	{
		for (int i = 0; i < 3; i++)
		{
			do_blink(100);
		}
		alert("I just booted up!");
		startup = false;
	}

	// handle memory reporting
	if ((millis() - memory_job_last) >= MEMORY_REPORTING_TIME * 1000)
	{
		report_free_memory();
		memory_job_last = millis();
	}


	// handle mode switching
	if (lastmode != automatic_mode)
	{
		logprint("[step] switch detected! is automatic mode enabled?" + String(automatic_mode));
	}
	lastmode = automatic_mode;

	if (automatic_mode)
	{
		if (millis() > last_switch + SWITCH_DELAY) {
			last_switch = millis();
			float current_temp = get_temp_bme();
			if (current_temp < target_temp) {
				heater_enabled = true;
			} else {
				heater_enabled = false;
			}

			//todo humidity check
		}
		
	}
//	else
//	{

	// debug prints
	logprint("[step] called, variables are: automatic=" + String(automatic_mode) +
			 ", heater =" + String(heater_enabled) + ", humidifier=" + String(humidifier_enabled));

	digitalWrite(RELAY_PIN_HEATER, heater_enabled);
	digitalWrite(RELAY_PIN_HUMIDIFIER, humidifier_enabled);
//	}
}

void alert(String text, bool do_reboot)
{
	logprint("[alert] " + text + ", with reboot = " + String(do_reboot));
	send_mqtt_msg(String(project_display_name) + "/alert", text);
	bool status = telegram_bot.sendMessage(TELEGRAM_CHAT_ID, text, "");
	logprint("[alert] telegram msg return status was " + String(status));
	if (do_reboot)
	{
		reboot();
	}
}