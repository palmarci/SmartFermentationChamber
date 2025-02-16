#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"
#include "utils.h"
#include "tasks.h"
#include "network.h"
#include "control.h"
#include "web.h"
#include "sensors.h"

TaskHandle_t task_handles[MAX_TASK_HANDLES] = {NULL};
int running_tasks = 0;

// reports mcu memory & task status and sends heartbeat on mqtt
void reporting_task(void *parameter)
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
		mqtt_send(MQTT_HEARTBEAT_TOPIC, "hello");
	}
}

// monitors & reconnects to wifi and mqtt
void network_task(void *parameter)
{
	int delay = 20 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		if (!wifi_connected())
		{
			wifi_init();
		}
		if (!mqtt_connected())
		{
			mqtt_init();
		}
		logprint("network checks done");
	}
}

// handles the auto switching of heater/humidifier based on sensor data
void autopilot_task(void *parameter)
{
	int delay = 5 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		if (get_autopilot_state())
		{
			autopilot_logic();
		}
	}
}

// updates the web interface
void web_update_task(void *parameter)
{
	int delay = 2 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		web_update();
	}
}

// restarts mcu after a given time
void periodic_reset_task(void *parameter)
{
	unsigned long delay = RESTART_AFTER * 60 * 60 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		reboot("periodic reset: " + String(RESTART_AFTER) + " hour reached");
	}
}

// handle special workaround to simulate a "press" on the humidifer's button
/*
void humdifier_helper_task(void *parameter)
{
	int delay = 0.5 * 1000;
	while (true)
	{
		if (humidifer_button_fire)
		{
			logprint("starting button push imitation");
			vTaskDelay(pdMS_TO_TICKS(delay)); // wait a little for relay
			digitalWrite(HUMIDIFER_PUSH_GATE, false); // close switch's pin to ground with transistor
			vTaskDelay(pdMS_TO_TICKS(delay)); // simulate button press
			digitalWrite(HUMIDIFER_PUSH_GATE, true); // pull high
			humidifer_button_fire = false;
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}
*/

// measure sensors
void sensor_task(void *parameter) {
	int delay = 3 * 1000;
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
	task_start(sensor_task, "sensor_task");
	task_start(reporting_task, "reporting_task");
	task_start(network_task, "network_task");
	task_start(web_update_task, "web_update_task");
	task_start(autopilot_task, "autopilot_task");
	//task_start(humdifier_helper_task, "humdifier_helper_task");
	task_start(periodic_reset_task, "periodic_reset_task");
}