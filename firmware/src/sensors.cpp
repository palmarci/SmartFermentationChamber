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
#include "control.h"

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

// https://gist.github.com/vsee/a51d2ebc7376bbd38f3d58c87c2b5d1b
float relative_to_abs_humidity(float temp, float hum_percentage) {
	float exponent = expf((17.67f * temp) / (temp + 243.5f)); 
	float absH = (6.112f * exponent * hum_percentage * 18.02f) / ((273.15f + temp) * 100.0f * 0.08314f);
	return absH;
}

float abs_to_relative_humidity(float temp, float absH) {
	float exponent = expf((17.67f * temp) / (temp + 243.5f)); 
	float hum_percentage = ((273.15f + temp) * 100.0f * 0.08314f * absH) / (6.112f * exponent * 18.02f);
	//logprint("abs_to_relative_humidity " + String(temp) + " C, " + String(absH) + " % -> " + String(hum_percentage));
	return hum_percentage;
}

void measure_sensors()
{
	bool all_good = true;
	check_sensor_timeout();

	float current_food_temp;
	float current_air_temp;
	float current_hum;

	// food temp
	dallas_sensors.requestTemperatures();
	current_food_temp = dallas_sensors.getTempCByIndex(0) + DALLAS_TEMP_OFFSET; // no checks here, i trust the user 
	if (!validate_temp_range(current_food_temp))
	{
		logprint("food temp read invalid: " + String(current_food_temp), LOG_WARNING);
		all_good = false;
	}
	
	// humidity
	float raw_hum = bme_sensor.readHumidity();
	logprint("raw humidity = " + String(raw_hum));
	if (validate_hum_range(raw_hum))
	{
		current_hum = raw_hum + BME_HUMIDITY_OFFSET; // add offset only after validation, this can in theory hit the limit
		// clamp it again after offsetting
		if (current_hum > 100) {
			current_hum = 100;
		} 
		if (current_hum < 0) {
			current_hum = 0;
		}
	}
	else
	{
		logprint("humidity read invalid: " + String(raw_hum), LOG_WARNING);
		all_good = false;
	}

	// air temp
	current_air_temp = bme_sensor.readTemperature() + BME_TEMP_OFFSET; // also no checks here
	if (!validate_temp_range(current_air_temp))
	{
		logprint("air temp read invalid: " + String(current_air_temp), LOG_WARNING);
		all_good = false;
	}

	if (all_good)
	{
		last_valid_timer = millis();
		last_hum = relative_to_abs_humidity(current_air_temp, current_hum);
		//last_hum = current_hum;
		last_food_temp = current_food_temp;
		last_air_temp = current_air_temp;
		mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/humidity", String(last_hum));
		mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/food_temp", String(last_food_temp));
		mqtt_send(String(MQTT_MEASUREMENT_TOPIC) + "/air_temp", String(last_air_temp));
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
	Wire.begin(I2C_SDA_PIN, I2C_SCL);

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
	String status_text = "Food temperature: " + String(last_food_temp) + " °C\n" +
						 "Chamber humidity: " + String(abs_to_relative_humidity(last_air_temp, last_hum)) + " % (" + String(last_hum) + " g/m^3)\n\n" + 

						 "Heater duty cycle: " + String(get_heater_duty_cycle()) + " %\n"+
						 "Humidifer duty cycle: " + String(get_humidifier_duty_cycle()) + " %\n\n"+

						 "Chamber temperature: " + String(last_air_temp)  + " °C\n"+
						 "Taget humidity: " + String(get_target_hum())  + " g/m^3\n";

	return status_text;
}