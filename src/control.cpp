#include <Arduino.h>
#include "config.h"

bool heater_state;
bool humidifier_state;

void set_heater(bool state)
{	
	heater_state = state;
	digitalWrite(RELAY_PIN_HEATER, state);
}

void set_humidifer(bool state)
{
	humidifier_state = state;
	digitalWrite(RELAY_PIN_HUMIDIFIER, state);
}

bool get_heater_state() {
	return heater_state;
}

bool get_humidifer_state() {
	return humidifier_state;
}

void control_init()
{
	pinMode(LED_PIN, OUTPUT);
	pinMode(RELAY_PIN_HEATER, OUTPUT);
	pinMode(RELAY_PIN_HUMIDIFIER, OUTPUT);
	set_heater(false);
	set_humidifer(false);
}