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
static float target_hum;
static float target_hum_rel;
static unsigned long last_relay_switch = 0;
static bool first_relay_switch = true;

static constexpr int DUTY_CYCLE_COUNT = (60 / SENSOR_MEASURE_PERIOD) * DUTY_CYCLE_LOGGING_TIME;
static int heater_duty_cycle_data[DUTY_CYCLE_COUNT];
static int humidifer_duty_cycle_data[DUTY_CYCLE_COUNT];
static float heater_duty_cycle = 0;
static float humidifier_duty_cycle = 0;
static int duty_cycle_index = 0;

static PredictionState predictor_temp = UNKNOWN;
static PredictionState predictor_hum = UNKNOWN;
float pred_avg_hum[PREDICTOR_AVERAGE_COUNT] = {-1};
float pred_avg_temp[PREDICTOR_AVERAGE_COUNT] = {-1};
int pred_avg_index = 0;


float get_target_temp()
{
	return target_temp;
}

float get_target_hum()
{
	return target_hum;
}

float get_target_hum_rel()
{
	return target_hum_rel;
}

void set_target_hum(float val)
{
	logprint("setting absolute target humidity to " + String(val));
	nvm_write_string(NVM_TARGET_HUM_ABS, String(val));
	target_hum = val;
}

void set_target_hum_rel(float val)
{
	logprint("setting relative target humidity to " + String(val));
	nvm_write_string(NVM_TARGET_HUM_REL, String(val));
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

void set_humidifier(bool state)
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

//	logprint("heater_duty_cycle = " + String(heater_duty_cycle) + " humidifier_duty_cycle = " + String(humidifier_duty_cycle));

	if (duty_cycle_index + 1 != DUTY_CYCLE_COUNT) {
		duty_cycle_index++;
	} else {
		duty_cycle_index = 0;
		logprint("duty cycle data restarts from beginning!", LOG_WARNING);
	}

}

void predictor_update(float current_hum, float current_temp) {
	float avg_hum, avg_temp;
	float sum_hum, sum_temp;
	int c = 0;

	// calc averages
	for (int i = 0; i < PREDICTOR_AVERAGE_COUNT; i++) {
		
		if (pred_avg_temp[i] != -1) {
			sum_temp += pred_avg_temp[i];
			c++;
		}
		
		if (pred_avg_hum[i] != -1) {
			sum_hum += pred_avg_hum[i];
		}

		avg_hum = sum_hum / c;
		avg_temp = sum_temp / c;
	}

	predictor_hum = current_hum > avg_hum ? GOING_UP : GOING_DOWN;
	predictor_temp = current_temp > avg_temp ? GOING_UP : GOING_DOWN;
	logprint("predictor_hum=" + String(predictor_hum) + ", avg_hum=" + String(avg_hum) + 
	" | predictor_temp=" + String(predictor_temp) + ", avg_temp=" + String(avg_temp));

}

void autopilot_step()
{
	auto target_temp = get_target_temp();
	auto target_hum = get_target_hum();

	pred_avg_hum[pred_avg_index] = last_hum;
	pred_avg_temp[pred_avg_index] = last_food_temp;

	if (pred_avg_index + 1 == PREDICTOR_AVERAGE_COUNT) {
		pred_avg_index = 0;
	} else {
		pred_avg_index++;
	}

	logprint("Autopilot step: food_temp = " + String(last_food_temp) + ", target_temp=" +
			 String(target_temp) + ", hum=" + String(last_hum) + ", target_hum=" + String(target_hum));

	// blink led to indicate that we are alive and well
	do_blink(300, 0);

	mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/heater_duty_cycle", String(get_heater_duty_cycle()));
	mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/humidifier_duty_cycle", String(get_humidifier_duty_cycle()));

	predictor_update(last_hum, last_food_temp);

	if (PREDICTOR_ENABLED) {

		if (last_food_temp < target_temp) {
			// Temperature is below target, normally heater on.
			// But if temperature is rising and has reached almost the target, turn heater off.
			if (predictor_temp == GOING_UP && last_food_temp >= (target_temp * (1.0f - PREDICTOR_PERCENTAGE))) {
				set_heater(false);
				logprint("Heater shut down due to reaching almost the target while rising", LOG_WARNING);
			} else {
				set_heater(true);
			}
		} else if (last_food_temp > target_temp) {
			// Temperature is above target, normally heater off.
			// But if temperature is falling and has dropped to almost the target, turn heater on.
			if (predictor_temp == GOING_DOWN && last_food_temp <= (target_temp * (1.0f + PREDICTOR_PERCENTAGE))) {
				set_heater(true);
				logprint("Heater turned on due to reaching almost the target while falling", LOG_WARNING);
			} else {
				set_heater(false);
			}
		} else {
			// At target temperature; maintain current state.
			set_heater(false);
		}
		
		// Humidifier logic (applies similar percentage-based logic):
		if (last_hum < target_hum) {
			// Humidity is below target, normally humidifier on.
			// If rising and reaching 96% of target humidity, shut it off.
			if (predictor_hum == GOING_UP && last_hum >= (target_hum * (1.0f - PREDICTOR_PERCENTAGE))) {
				set_humidifier(false);
				logprint("Humidifier shut down due to reaching 96% of target humidity while rising", LOG_WARNING);
			} else {
				set_humidifier(true);
			}
		} else if (last_hum > target_hum) {
			// Humidity is above target, normally humidifier off.
			// If falling and reaching 104% of target humidity, turn it on.
			if (predictor_hum == GOING_DOWN && last_hum <= (target_hum * (1.0f + PREDICTOR_PERCENTAGE))) {
				set_humidifier(true);
				logprint("Humidifier turned on due to reaching 104% of target humidity while falling", LOG_WARNING);
			} else {
				set_humidifier(false);
			}
		} else {
			// At target humidity; maintain current state.
			set_humidifier(false);
		}
		
		
	} else {

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
			set_humidifier(true);
		}
		else
		{
			set_humidifier(false);
		}
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

		// init duty cycle logging
		for (int i = 0; i < DUTY_CYCLE_COUNT; i++) {
			humidifer_duty_cycle_data[i] = -1;
			heater_duty_cycle_data[i] = -1;
		}

		autopilot_step();
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
	float hum_targ = nvm_read_string(NVM_TARGET_HUM_ABS).toFloat();
	float hum_targ_rel = nvm_read_string(NVM_TARGET_HUM_REL).toFloat();

	set_target_hum(hum_targ);
	set_target_hum_rel(hum_targ_rel);
	set_target_temp(temp_targ);
}