#include <Arduino.h>

#include <ESPUI.h>
#include <esp_heap_caps.h>

#include "config.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>


//WiFiClientSecure wifi_client_secure;

//UniversalTelegramBot telegram_bot(TELEGRAM_BOT_TOKEN, wifi_client_secure);

// globals

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

#include "control.h"

String log_level_to_string(int level) {
	switch (level) {
		case LOG_DEBUG:
			return "debug";
		case LOG_WARNING:
			return "warning";
		case LOG_PANIC:
			return "panic";
	}
}

void log(String text, int level = LOG_DEBUG)
{
	String level_text = log_level_to_string(level);
	// TODO calculate real time
	String data = ("[" + level_text + " - " + String(millis()) + "] " + text);
	// TODO add mqtt logging & check if we are connected first
	Serial.println(data.c_str());
}

void halt(String reason)
{
	log("HALTING! reason: " + reason);
	set_heater(false);
	set_humidifer(false);
	while (true)
	{
		do_blink(500);
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

void report_free_memory()
{
	uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
	uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
	float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
	log("[memory] " + String(percentageHeapFree) + " % free of " + String(totalHeapBytes / 1000) + "k");
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
	// log("STARTING VERSION v" + String(VERSION));

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

	// setup telegram bot
	wifi_client_secure.setCACert(TELEGRAM_CERTIFICATE_ROOT);
	telegram_bot.waitForResponse = TELEGRAM_WAITFOR;
}



void (*resetFunc)(void) = 0;
void reset()
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
		log("[step] switch detected! is automatic mode enabled?" + String(automatic_mode));
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
	log("[step] called, variables are: automatic=" + String(automatic_mode) +
			 ", heater =" + String(heater_enabled) + ", humidifier=" + String(humidifier_enabled));

	digitalWrite(RELAY_PIN_HEATER, heater_enabled);
	digitalWrite(RELAY_PIN_HUMIDIFIER, humidifier_enabled);
//	}
}

void reboot(String error_message)
{
	//TODO
	log("[alert] " + text + ", with reboot = " + String(do_reboot));
	send_mqtt_msg(String(project_display_name) + "/alert", text);
	bool status = telegram_bot.sendMessage(TELEGRAM_CHAT_ID, text, "");
	log("[alert] telegram msg return status was " + String(status));
	if (do_reboot)
	{
		reboot();
	}
}