#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>

#include "sensors.h"
#include "utils.h"
#include "config.h"

OneWire onewire_bus(ONEWIRE_BUS_PIN);
DallasTemperature dallas_sensors(&onewire_bus);
Adafruit_BME280 bme_sensor;

float last_hum;
float last_food_temp;
float last_air_temp;

unsigned long last_hum_valid;
unsigned long last_food_temp_valid;
unsigned long last_air_temp_valid;

bool validate_temp_range(float input)
{
	if (input < INVALID_MINIMUM_TEMP || input > INVALID_MAX_TEMP)
	{
		return false;
	}
	return true;
}

bool validate_hum_range(float in)
{
	if (in < 0 || in > 100)
	{
		return false;
	}
	return true;
}

bool bme_check_alive()
{
	sensors_event_t last_event;
	bme_sensor.getTemperatureSensor()->getEvent(&last_event);
	if (last_event.data[0] > 160 || isnan(last_event.data[0]))
	{
		// reboot("bme sensor may be disconnected");
		return false;
	}
	return true;
}

float get_humidity()
{
	check_sensor_timeout(last_hum_valid);

	if (!bme_check_alive())
	{
		logprint("bme humidity sensor seems to be dead, returning last avg", LOG_WARNING);
		return last_hum;
	}

	float hum = bme_sensor.readHumidity();
	if (!validate_hum_range(hum))
	{
		halt("read humidity is outside of valid range");
	}
	last_hum = hum;
	last_hum_valid = millis();
	return last_hum;
}

void check_sensor_timeout(unsigned long last_valid)
{
	unsigned long now = millis();
	unsigned long limit = last_valid + (SENSOR_MEASUREMENT_TIMEOUT * 1000);
	if (limit < now)
	{
		halt("could not read data from sensor for more than the set timeout");
	}
}

float get_air_temp()
{
	check_sensor_timeout(last_air_temp_valid);

	if (!bme_check_alive())
	{
		logprint("bme temp sensor seems to be dead, returning last avg", LOG_WARNING);
		return last_air_temp;
	}

	float temp = bme_sensor.readTemperature();

	if (!validate_temp_range(temp))
	{
		halt("bme sensor value is outside of valid range");
	}
	last_air_temp = temp;
	last_air_temp_valid = millis();

	return last_air_temp;
}

float get_food_temp()
{
	check_sensor_timeout(last_food_temp_valid);

	dallas_sensors.requestTemperatures();
	float temp = dallas_sensors.getTempCByIndex(0);

	if (temp < -126)
	{
		logprint("dallas sensor seems to be dead, returning last avg", LOG_WARNING);
		return last_food_temp;
	}

	if (!validate_temp_range(temp))
	{
		halt("dallas sensor value is outside of valid range");
	}
	last_food_temp = temp;
	last_food_temp_valid = millis();
	return last_food_temp;
}

void init_dallas()
{
	dallas_sensors.begin();
	if (dallas_sensors.getDeviceCount() < 1)
	{
		halt("no dallas devices found on bus");
	}
	dallas_sensors.setResolution(12);
	logprint("set dallas sensor resolution to maximum");
}

void init_bme()
{
	Wire.begin(BME_I2C_SDA_PIN, BME_I2C_SCL_PIN);
	if (!bme_sensor.begin(BME_I2C_ID, &Wire))
	{
		halt("no BME280 sensor found");
	}
	else
	{
		logprint("BME280 sensor initialized successfulyl!");
	}
}

void sensors_init()
{
	init_dallas();
	init_bme();
}

String get_sensor_status_text()
{
	String status_text = "Temperature inside the chamber: " + String(get_air_temp()) + "\n" +
						 "Temperature inside the food: " + String(get_food_temp()) + "\n" +
						 "Humdity: " + String(get_humidity()) + "%";
	return status_text;
}