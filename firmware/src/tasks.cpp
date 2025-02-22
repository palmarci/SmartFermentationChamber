#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
//#include <ArduinoOTA.h>

#include "config.h"
#include "utils.h"
#include "tasks.h"
#include "network.h"
#include "control.h"
#include "web.h"
#include "sensors.h"
#include "ota.h"
#include "nvm.h"

TaskHandle_t task_handles[MAX_TASK_HANDLES] = {NULL};
static int running_tasks = 0;
static unsigned long restart_delay_ms = (unsigned long)RESTART_AFTER_HOURS * 60UL * 60UL * 1000UL;

// reports mcu memory & task status and sends heartbeat on mqtt, resets if needed
void housekeeping_task(void *parameter)
{
	int delay = 60 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
		uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
		float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
		String mem_text = "free memory: " + String(percentageHeapFree) + " % free of " + String(totalHeapBytes / 1000) + "k";
		unsigned int running_tasks = uxTaskGetNumberOfTasks();
		String tasks_text = "currently running tasks: " + String(running_tasks);
		logprint(tasks_text);
		logprint(mem_text);

		if (millis() > restart_delay_ms) {
			reboot("periodic reset: " + String(restart_delay_ms) + " ms reached!");
		}
	}
}

void network_reconnector_task(void *parameter)
{
	int delay = 5 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));

		if (!wifi_is_connected())
		{
			Serial.println("we are not connected to wifi!");
			String ssid = nvm_read_string(NVM_WIFI_SSID);
			String pw = nvm_read_string(NVM_WIFI_PW);
			wifi_connect(ssid, pw);
		}
		if (wifi_is_connected() && !mqtt_connected())
		{
			Serial.println("we are not connected to mqtt!");
			mqtt_init();
		}

		if (!wifi_is_connected() || !mqtt_connected()) {
			Serial.println("something went wrong during network reconnect!");
		} else {
			Serial.println("all reconnected!");
		}
	}
}


// handles the auto switching of heater/humidifier based on sensor data
void autopilot_task(void *parameter)
{
	int delay = SENSOR_MEASURE_PERIOD * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		if (get_autopilot_state())
		{
			autopilot_step();
		}
		duty_cycle_update();
	}
}

// updates the web interface
void web_update_task(void *parameter)
{
	int delay = 2 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		if (wifi_is_connected()) {
			web_update();
		}
		
	}
}


// measure sensors
void sensor_measure_task(void *parameter) {
	int delay = SENSOR_MEASURE_PERIOD * 1000;
	while (true)
	{
		measure_sensors();
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

void set_task_handler(TaskHandle_t handle)
{
	if (running_tasks < MAX_TASK_HANDLES)
	{
		task_handles[running_tasks++] = handle;
	}
	else
	{
		halt("limit of max tasks reached!", 500, 500);
	}
}

void task_start(void (*taskFunction)(void *), String name)
{
	uint32_t defaut_stack_size = STACK_SIZE;
	TaskHandle_t taskHandle = NULL;
	xTaskCreate(taskFunction, name.c_str(), defaut_stack_size, NULL, 1, &taskHandle);
	set_task_handler(taskHandle);
	logprint("started task " + name);
}

void stop_all_tasks()
{
	for (int i = 0; i < running_tasks; i++)
	{
		if (task_handles[i] != NULL)
		{
			vTaskDelete(task_handles[i]);
			logprint("stopping task " + String(i));
			task_handles[i] = NULL;
		}
	}
}

void tasks_init()
{
	logprint("*** tasks_init ***");
	task_start(sensor_measure_task, "sensor_measure_task");
	task_start(housekeeping_task, "housekeeping_task");
	if (!wifi_is_in_ap_mode()) {
		task_start(network_reconnector_task, "network_reconnector_task");
	} else {
		Serial.println("skipping network reconnector task since we are in AP mode!");
	}
	task_start(web_update_task, "web_update_task");

	if (OTA_ENABLED) {
		task_start(ota_task, "ota_task");
	}

	vTaskDelay(pdMS_TO_TICKS(100)); // ensure autopilot starts up after sensor_measure_task ran, so we get fresh data
	task_start(autopilot_task, "autopilot_task");
}