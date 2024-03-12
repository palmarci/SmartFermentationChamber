#include <Arduino.h>

#include "sensors.h"
#include "utils.h"

bool validate_temp_range(float input)
{
	if (input < INVALID_MINIMUM_TEMP || input > INVALID_MAX_TEMP)
	{
		return false;
	}
	return true;
}

bool validate_hum_range(float in) {
	if (in < 0 || in > 100) {
		return false;
	} 
	return true;
}

float get_humidity()
{
	bme_check_alive();
	float hum = bme_sensor.readHumidity();
	log("[bme] hum=" + String(hum));
	if (!validate_hum_range(hum)) {
		halt("read humidity is outside of valid range");
	}
	return hum;
}

void bme_check_alive()
{
	sensors_event_t last_event;
	bme_sensor.getTemperatureSensor()->getEvent(&last_event);
	if (last_event.data[0] > 160 || isnan(last_event.data[0]))
	{ 
		reboot("bme sensor may be disconnected");
	}
}

float get_air_temp()
{
	bme_check_alive();

	float temp = bme_sensor.readTemperature();
	log("[bme] temp=" + String(temp));

	if (!validate_temp_range(temp))
	{
		halt("bme sensor value is outside of valid range");
	}

	return temp;
}


float get_food_temp()
{
	dallas_sensors.requestTemperatures();
	float temp = dallas_sensors.getTempCByIndex(0);
	log("[dallas] read temperature " + String(temp));
	if (temp < -126)
	{
		reboot("dallas sensor may be disconnected or faulty, rebooting...");
	}

	if (!validate_temp_range(temp))
	{
		halt("dallas sensor read invalid value");
	}

	return temp;
}