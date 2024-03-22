#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"
#include "utils.h"
#include "tasks.h"
#include "network.h"
#include "control.h"
#include "web.h"

TaskHandle_t task_handles[MAX_TASK_HANDLES] = {NULL};
int running_tasks = 0;

void set_task_handler(TaskHandle_t handle)
{
	if (running_tasks < MAX_TASK_HANDLES)
	{
		task_handles[running_tasks++] = handle;
	}
	else
	{
		halt("limit of max tasks reached!");
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

// reports current status and sends heartbeat on mqtt
void reporting_task(void *parameter)
{
	int delay = 60 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));

		uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
		uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
		float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
		String mem_text = "free memory: " + String(percentageHeapFree) + " % free of " +
						  String(totalHeapBytes / 1000) + "k";
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

// handles the auto switching of relays based on sensor data
void autopilot_task(void *parameter)
{
	int delay = 15 * 1000;
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

// restarts device after given time
void restart_task(void *parameter)
{
	unsigned long delay = RESTART_AFTER * 60 * 60 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		reboot(String(RESTART_AFTER) + " hour restart timeout reached");
	}
}

// handle special workaround to "press" humidifer's button
void humdifier_helper_task(void *parameter)
{
	int wait_for_init = 2 * 1000; // wait a little for relay to activate & humidifer to initialize
	int delay = 100;
	while (true)
	{
		if (humidifer_helper_do)
		{
			vTaskDelay(pdMS_TO_TICKS(wait_for_init));
			digitalWrite(HUMIDIFER_PUSH_GATE, true);
			vTaskDelay(pdMS_TO_TICKS(500)); // simulate button press with fet between button pins
			digitalWrite(HUMIDIFER_PUSH_GATE, false);
			humidifer_helper_do = false;
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
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
	task_start(reporting_task, "reporting_task");
	task_start(network_task, "network_task");
	task_start(web_update_task, "web_update_task");
	task_start(autopilot_task, "autopilot_task");
	task_start(humdifier_helper_task, "humdifier_helper_task");
}