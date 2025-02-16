#include <Arduino.h>
#include "config.h"
#include "utils.h"
#include "nvm.h"
#include "sensors.h"
#include "tasks.h"
#include "control.h"

bool heater_state;
bool humidifier_state;
bool autopilot_state;
float target_temp;
float target_hum;

//bool humidifer_button_fire = false;
 
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
	nvm_write_string(NVM_TARGET_HUM, String(val));
	target_hum = val;
}

void set_target_temp(float val)
{
	logprint("setting target temp to " + String(val));
	nvm_write_string(NVM_TARGET_TEMP, String(val));
	target_temp = val;
}

void set_heater(bool state)
{
	logprint("setting heater to " + String(state));
	heater_state = state;
	if (PIN_HEATER_INVERT)
	{
		state = !state;
	}
	digitalWrite(PIN_HEATER, state);
}

void set_humidifer(bool state)
{
//	if (humidifier_state && state) {
//		logprint("skipping humidifer turn on, already turned on");
//		return;
//	}
	logprint("setting humidifier to " + String(state));
	humidifier_state = state;

	// handle relay stuff
	if (PIN_HUMIDIFIER_INVERT)
	{
		state = !state;
	}
	digitalWrite(PIN_HUMIDIFIER, state);

//	if (state) {
		// no need to sleep here for relay closing, helper task takes care of it
		//humidifer_button_fire = true;
//	}

}

bool get_heater_state()
{
	return heater_state;
}

bool get_humidifer_state()
{
	return humidifier_state;
}

bool get_autopilot_state()
{
	return autopilot_state;
}

void autopilot_logic()
{
	auto target_temp = get_target_temp();
	auto target_hum = get_target_hum();
	logprint("Autopilot step: food_temp = " + String(last_food_temp) + ", target_temp=" + \
	String(target_temp) + ", hum=" + String(last_hum) + ", target_hum=" + String(target_hum));


	if (last_food_temp < target_temp)
	{
		set_heater(true);
	}
	else
	{
		set_heater(false);
	}

	if (last_hum < target_hum)
	{
		set_humidifer(true);
	}
	else
	{
		set_humidifer(false);
	}
}

void set_autopilot(bool state)
{
	logprint("setting autopilot to " + String(state));
	autopilot_state = state;
	if (autopilot_state)
	{
		logprint("autopilot was just enabled, running logic...");
		// run it instantly for smoother ui experience
		// task is constantly running every X seconds, so this is needed here only once
		autopilot_logic();
	}
}

void target_values_init()
{
	logprint("*** target_values_init ***");
	// set targets
	String hum_str = nvm_read_string(NVM_TARGET_HUM);
	String temp_str = nvm_read_string(NVM_TARGET_TEMP);
	//logprint("read target hum from nvm: " + hum_str);
	//logprint("read target temp from nvm: " + temp_str);
	set_target_hum(hum_str.toFloat());
	set_target_temp(temp_str.toFloat());
}