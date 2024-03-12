#include <Arduino.h>
#include <ESPUI.h>
#include <esp_heap_caps.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "config.h"
#include "network.h"
#include "control.h"

/*
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
*/

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
	Serial.println(data.c_str());
	if (mqtt_connected()) {
		mqtt_send(MQTT_LOG_TOPIC, data);
	}
}

String bool_to_str(bool val) //for web ui
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

void do_blink(int time_delay)
{
	digitalWrite(LED_PIN, HIGH);
	delay(time_delay);
	digitalWrite(LED_PIN, LOW);
	delay(time_delay);
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

// SHUTDOWN LOGIC

void halt(String reason)
{
	log("HALTING! reason: " + reason);
	set_heater(false);
	set_humidifer(false);
	//TODO kill all tasks
	while (true)
	{
		do_blink(500);
	}
}

void (*resetFunc)(void) = 0;
void reset()
{
	resetFunc();
}

void reboot(String error_message)
{
	log("REBOOT! reason: " + error_message);
	reset();
}