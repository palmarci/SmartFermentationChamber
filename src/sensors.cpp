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

float get_humidity()
{
	bme_check_alive();
	float hum = bme_sensor.readHumidity();
	log("[bme] hum=" + String(hum));
	if (!validate_hum_range(hum))
	{
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

void init_dallas()
{
	dallas_sensors.begin();
	if (dallas_sensors.getDeviceCount() < 1)
	{
		halt("no dallas devices found on bus");
	}
	dallas_sensors.setResolution(12);
}

void init_bme()
{
	Wire.begin(BME_I2C_SDA_PIN, BME_I2C_SCL_PIN);
	if (!bme_sensor.begin(BME_I2C_ID, &Wire))
	{
		halt("no BME280 sensor found");
	}
}

void sensors_init()
{
	init_dallas();
	init_bme();
}

String get_status_text()
{
	String status_text = "Temperature inside the chamber: " + String(get_air_temp()) + "\n" +
						 "Temperature inside the food: " + String(get_food_temp()) + "\n" +
						 "Humdity: " + String(get_humidity()) + "%";
	return status_text;
}