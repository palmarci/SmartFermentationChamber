#include <Arduino.h>
#include "config.h"
#include "utils.h"
#include "nvm.h"
#include "sensors.h"
#include "tasks.h"
#include "control.h"
#include "utils.h"
#include "network.h"

static bool heater_state;
static bool humidifier_state;
static bool autopilot_state;
static float target_temp;
static float target_hum_rel;
static unsigned long last_relay_switch = 0;
static bool first_relay_switch = true;

static constexpr int DUTY_CYCLE_COUNT = (60 / SENSOR_MEASURE_PERIOD) * DUTY_CYCLE_LOGGING_TIME;
static int heater_duty_cycle_data[DUTY_CYCLE_COUNT];
static int humidifer_duty_cycle_data[DUTY_CYCLE_COUNT];
static float heater_duty_cycle = 0;
static float humidifier_duty_cycle = 0;
static int duty_cycle_index = 0;

float get_target_temp()
{
	return target_temp;
}

float get_target_hum()
{
	return target_hum_rel;
}

void set_target_hum_abs(float val)
{
	logprint("setting absolute target humidity to " + String(val));
	nvm_write_string(NVM_TARGET_HUM, String(val));
	target_hum_rel = val;
}

void set_target_temp(float val)
{
	logprint("setting target temp to " + String(val));
	nvm_write_string(NVM_TARGET_TEMP, String(val));
	target_temp = val;
}

void set_heater(bool state, bool force_mode=false)
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
	if (relay_switch_delta > RELAY_PROTECTION_DELAY * 60 || first_relay_switch || force_mode)
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

String debug_print_duty_cycle(int *data) {
	String toret = "[";
	for (int i = 0; i < DUTY_CYCLE_COUNT; i++) {
		toret += String(data[i]) + ", "; 
	}
	toret += "]";
	return toret;
}

void duty_cycle_update() {
	int heater_value = get_heater_state() ? 1 : 0;
	int humidifier_value = get_humidifer_state() ? 1 : 0;

	heater_duty_cycle_data[duty_cycle_index] = heater_value;
	humidifer_duty_cycle_data[duty_cycle_index] = humidifier_value;

	float heat_sum = 0;
	float hum_sum = 0;
	int c = 0;

	//logprint("heater_duty_cycle_data = " + debug_print_duty_cycle(heater_duty_cycle_data));
	//logprint("humidifer_duty_cycle_data = " + debug_print_duty_cycle(humidifer_duty_cycle_data));

	for (int i = 0; i < DUTY_CYCLE_COUNT; i++) {
		if (heater_duty_cycle_data[i] != -1) {
			heat_sum += heater_duty_cycle_data[i];
			c += 1;
		}

		if (humidifer_duty_cycle_data[i] != -1) {
			hum_sum += humidifer_duty_cycle_data[i];
		}
	}

//	logprint("duty cycle found " + String(c) + " values to work with");

	if (c != 0) {
		heater_duty_cycle = heat_sum / c;
		humidifier_duty_cycle = hum_sum / c;
	} else {
		logprint("duty cycle value is zero!", LOG_WARNING);
	}

	logprint("heater_duty_cycle = " + String(heater_duty_cycle) + " humidifier_duty_cycle = " + String(humidifier_duty_cycle));

	if (duty_cycle_index + 1 != DUTY_CYCLE_COUNT) {
		duty_cycle_index++;
	} else {
		duty_cycle_index = 0;
		logprint("duty cycle data restarts from beginning!");
	}

	mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/heater_duty_cycle", String(heater_duty_cycle));
	mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/humidifier_duty_cycle", String(humidifier_duty_cycle));

}

void autopilot_logic()
{
	auto target_temp = get_target_temp();
	auto target_hum = get_target_hum();

	logprint("Autopilot step: food_temp = " + String(last_food_temp) + ", target_temp=" +
			 String(target_temp) + ", hum=" + String(last_hum) + ", target_hum=" + String(target_hum));

	// blink led to indicate that we are alive and well
	do_blink(300, 0);

	// check if heater is necessary
	if (last_food_temp < target_temp)
	{
		set_heater(true);
	}
	else
	{
		set_heater(false);
	}

	// check if humidifier is necessary
	if (last_hum < target_hum)
	{
		set_humidifer(true);
	}
	else
	{
		set_humidifer(false);
	}

	duty_cycle_update();

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

		// init duty cycle logging
		for (int i = 0; i < DUTY_CYCLE_COUNT; i++) {
			humidifer_duty_cycle_data[i] = -1;
			heater_duty_cycle_data[i] = -1;
		}

		autopilot_logic();
	}
}

int get_heater_duty_cycle() {
	return heater_duty_cycle * 100;
}

int get_humidifier_duty_cycle() {
	return humidifier_duty_cycle * 100;
}

void control_init()
{
	logprint("*** control_init ***");

	float temp_targ = nvm_read_string(NVM_TARGET_TEMP).toFloat();
	float hum_targ = nvm_read_string(NVM_TARGET_HUM).toFloat();

	set_target_hum_abs(relative_to_abs_humidity(temp_targ, hum_targ));
	set_target_temp(temp_targ);

}