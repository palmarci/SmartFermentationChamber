#include <Arduino.h>
#include <ESPUI.h>
#include <esp_heap_caps.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "config.h"
#include "network.h"
#include "control.h"
#include "tasks.h"

String log_level_to_string(int level)
{
	switch (level)
	{
	case LOG_DEBUG:
		return "debug";
	case LOG_WARNING:
		return "warning";
	case LOG_PANIC:
		return "panic";
	}
	return "unknown";
}

void logprint(String text, int level = LOG_DEBUG)
{
	String level_text = log_level_to_string(level);
	String data = ("[" + level_text + " - " + String(millis()) + "] " + text);
	Serial.println(data.c_str());
	mqtt_send(MQTT_LOG_TOPIC, data);
}

String bool_to_str(bool val) // for web ui
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

void do_blink(int delay_between, int delay_after)
{
	digitalWrite(LED_PIN, HIGH);
	delay(delay_between);
	digitalWrite(LED_PIN, LOW);
	delay(delay_after);
}

void halt(String reason, int delay_between, int delay_after)
{
	logprint("HALTING! reason: " + reason, LOG_PANIC);
	set_heater(false);
	set_humidifer(false);
	stop_all_tasks();
	while (true)
	{
		do_blink(delay_between, delay_after);
	}
}

void (*resetFunc)(void) = 0;
void reset()
{
	resetFunc();
}

void reboot(String error_message)
{
	logprint("REBOOT! reason: " + error_message, LOG_PANIC);
	reset();
}

String remove_leading_slash(String input)
{
	int i;
	int len = input.length();

	// Find the index of the first non-slash character
	for (i = 0; i < len && input.charAt(i) == '/'; ++i);

	// If the first character is not '/', return the original string
	if (i == 0) {
		return input;
	}

	// Return the substring starting from the first non-slash character
	return input.substring(i);
}