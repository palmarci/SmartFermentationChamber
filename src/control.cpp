#include <Arduino.h>
#include "config.h"
#include "utils.h"
#include "nvm.h"

bool heater_state;
bool humidifier_state;
bool autopilot_state;
float target_temp;
float target_hum;

float get_target_temp()
{
	return target_temp;
}

float get_target_hum()
{
	return target_hum;
}

void set_target_hum(float val)
{
	logprint("setting target humidity to " + String(val));
	target_hum = val;
}

void set_target_temp(float val)
{
	logprint("setting target temp to " + String(val));
	target_temp = val;
}

void set_heater(bool state)
{
	heater_state = state;
	logprint("setting heater to " + String(state));
	digitalWrite(RELAY_PIN_HEATER, state);
}

void set_humidifer(bool state)
{
	humidifier_state = state;
	logprint("setting humidifier to " + String(state));
	digitalWrite(RELAY_PIN_HUMIDIFIER, state);
}

bool get_heater_state()
{
	return heater_state;
}

bool get_humidifer_state()
{
	return humidifier_state;
}

void set_autopilot(bool state)
{
	logprint("setting autopilot to " + String(state));
	autopilot_state = state;
}

bool get_autopilot_state()
{
	return autopilot_state;
}

void control_init()
{
	pinMode(LED_PIN, OUTPUT);
	pinMode(RELAY_PIN_HEATER, OUTPUT);
	pinMode(RELAY_PIN_HUMIDIFIER, OUTPUT);
	String hum_str = nvm_read_string(NVM_TARGET_HUM);
	String temp_str = nvm_read_string(NVM_TARGET_TEMP);
	logprint("read target hum from nvm: " + hum_str);
	logprint("read target temp from nvm: " + temp_str);
	set_target_hum(hum_str.toFloat());
	set_target_temp(temp_str.toFloat());
	set_heater(false);
	set_humidifer(false);
	set_autopilot(DEFAULT_AUTOPILOT_ENABLED);
}