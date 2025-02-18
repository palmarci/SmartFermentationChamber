#include <Arduino.h>
#include "config.h"
#include "utils.h"
#include "nvm.h"
#include "sensors.h"
#include "tasks.h"
#include "control.h"
#include "pid.h"
#include "utils.h"

static unsigned long last_autopilot = 0;
static bool heater_state;
static bool humidifier_state;
static bool autopilot_state;
static float target_temp;
static float target_hum_abs;
static float target_hum_rel;
static unsigned long last_relay_switch = 0;
static bool first_relay_switch = true;

// bool humidifer_button_fire = false;

float get_target_temp()
{
	return target_temp;
}

float get_target_hum_rel()
{
	return target_hum_rel;
}

void set_target_hum_rel(float val)
{
	logprint("setting relative target humidity to " + String(val));
	nvm_write_string(NVM_TARGET_HUM_REL, String(val));
	target_hum_rel = val;
}

float get_target_hum_abs()
{
	return target_hum_abs;
}

void set_target_hum_abs(float val)
{
	logprint("setting absolute target humidity to " + String(val));
	nvm_write_string(NVM_TARGET_HUM_ABS, String(val));
	target_hum_abs = val;
}

void set_target_temp(float val)
{
	logprint("setting target temp to " + String(val));
	nvm_write_string(NVM_TARGET_TEMP, String(val));
	target_temp = val;
}

void set_heater(bool state)
{
	if (heater_state != state)
	{
		logprint("setting heater to " + String(state));
	}
	else
	{
		return;
	}

	int relay_switch_delta = (millis() - last_relay_switch) / 1000;
	if (relay_switch_delta > RELAY_PROTECTION_DELAY * 60 || first_relay_switch)
	{
		heater_state = state;

		if (PIN_HEATER_INVERT)
		{
			state = !state;
		}

		digitalWrite(PIN_HEATER, state);
		last_relay_switch = millis();
		first_relay_switch = false;
	}
	else
	{
		logprint("skipping relay switch due to protection! delta was " + String(relay_switch_delta) + " sec", LOG_WARNING);
		return;
	}
}

void set_humidifer(bool state)
{
	if (humidifier_state != state)
	{
		logprint("setting humidifier to " + String(state));
	} // else return is not necessary since it is just a gpio, we can safely set it no matter what

	humidifier_state = state;

	if (PIN_HUMIDIFIER_INVERT)
	{
		state = !state;
	}

	digitalWrite(PIN_HUMIDIFIER, state);
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
	auto target_hum = get_target_hum_abs();
	logprint("Autopilot step: food_temp = " + String(last_food_temp) + ", target_temp=" +
			 String(target_temp) + ", hum=" + String(last_hum) + ", target_hum=" + String(target_hum));

	do_blink(500, 0);

	if (PID_ENABLED)
	{

		float dt_seconds = (millis() - last_autopilot) / 1000.0f;
		set_heater(pid_control(target_temp, last_food_temp, dt_seconds));
	}
	else
	{

		// check if heater is necessary
		bool new_heater_state;

		if (last_food_temp < target_temp)
		{
			new_heater_state = true;
		}
		else
		{
			new_heater_state = false;
		}

		// check if we are not warming up too rapidly
		if (last_air_temp > 1.5 * target_temp)
		{
			logprint("forcing heater to off due to rapid warmup!", LOG_WARNING);
			new_heater_state = false;
		}

		set_heater(new_heater_state);
	}

	// TODO: currently humidity is not pid controlled
	if (last_hum < target_hum)
	{
		set_humidifer(true);
	}
	else
	{
		set_humidifer(false);
	}

	last_autopilot = millis();
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

void control_init()
{
	logprint("*** control_init ***");

	String hum_str_abs = nvm_read_string(NVM_TARGET_HUM_ABS);
	set_target_hum_abs(hum_str_abs.toFloat());

	String hum_str_rel = nvm_read_string(NVM_TARGET_HUM_REL);
	set_target_hum_rel(hum_str_rel.toFloat());

	String temp_str = nvm_read_string(NVM_TARGET_TEMP);
	set_target_temp(temp_str.toFloat());

	float kp = nvm_read_string(NVM_PID_KP).toFloat();
	float ki = nvm_read_string(NVM_PID_KI).toFloat();
	float kd = nvm_read_string(NVM_PID_KD).toFloat();

	pid_init(kp, ki, kd);
}