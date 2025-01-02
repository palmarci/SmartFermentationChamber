#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>

#include "sensors.h"
#include "utils.h"
#include "config.h"
#include "network.h"

OneWire onewire_bus(ONEWIRE_BUS_PIN);
DallasTemperature dallas_sensors(&onewire_bus);
Adafruit_BME280 bme_sensor;

#define INVALID_DEFAULT_VALUE 999

unsigned long last_valid_timer = INVALID_DEFAULT_VALUE; // overflow is taken care of by the periodic reset task
float last_hum = INVALID_DEFAULT_VALUE;
float last_food_temp = INVALID_DEFAULT_VALUE;
float last_air_temp = INVALID_DEFAULT_VALUE;

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

void check_sensor_timeout()
{
	unsigned long now = millis();
	unsigned long limit = last_valid_timer + (SENSOR_MEASUREMENT_TIMEOUT * 1000);
	if (limit < now)
	{
		reboot("sensor data timeout reached"); // was halt() instead
	}
}

void measure_sensors()
{
	bool all_good = true;
	check_sensor_timeout();

	// humidity
	float hum = bme_sensor.readHumidity();
	if (validate_hum_range(hum))
	{
		last_hum = hum;
		mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/humidity", String(last_hum));
	}
	else
	{
		all_good = false;
	}

	// food temp
	dallas_sensors.requestTemperatures();
	float food_temp = dallas_sensors.getTempCByIndex(0);
	if (validate_temp_range(food_temp))
	{
		last_food_temp = food_temp;
		mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/food_temp", String(last_food_temp));
	}
	else
	{
		all_good = false;
	}

	// air temp
	float air_temp = bme_sensor.readTemperature();
	if (validate_temp_range(air_temp))
	{
		last_air_temp = air_temp;
		mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/air_temp", String(last_air_temp));
	}
	else
	{
		all_good = false;
	}

	if (all_good)
	{
		last_valid_timer = millis();
	}
	else
	{
		logprint("invalid data from sensor(s), invalidated current measurement", LOG_WARNING);
	}
}

void init_dallas()
{
	dallas_sensors.begin();
	int count = dallas_sensors.getDeviceCount();
	if (count < 1)
	{
		halt("no dallas devices found on bus", 500, 0);
	}
	logprint("found " + String(count) + " dallas sensors on bus");
	dallas_sensors.setResolution(12);
	logprint("set dallas sensor resolution to 12 bits");
}

void init_bme()
{
	logprint("initializing bme sensor...");
	Wire.end();
	Wire.begin(BME_I2C_SDA_PIN, BME_I2C_SCL_PIN);

	if (!bme_sensor.begin(BME_I2C_ID, &Wire))
	{
		halt("failed to initialize BME280 sensor!", 1000, 0);
	}
	else
	{
		logprint("BME280 sensor initialized successfully!");
	}
}

void sensors_init()
{
	logprint("*** sensors_init ***");
	init_dallas();
	init_bme();
	measure_sensors();

	if (INVALID_DEFAULT_VALUE == last_valid_timer)
	{
		halt("read invalid data from sensors at startup", 500, 1000);
	}
}

String get_sensor_status_text()
{
	String status_text = "Food temperature: " + String(last_food_temp) + "\n\n" +
						 "Chamber temperature: " + String(last_air_temp) + "\n" +
						 "Chamber humdity: " + String(last_hum) + "%";
	return status_text;
}