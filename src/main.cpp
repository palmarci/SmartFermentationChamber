#include "utils.h"
#include "control.h"
#include "nvm.h"
#include "network.h"
#include "sensors.h"
#include "web.h"
#include "main.h"
#include "config.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TaskHandle_t task_handles[MAX_TASK_HANDLES] = {NULL};
int running_tasks = 0;

// reports current status and sends heartbeat on mqtt
void reporting_task(void *parameter)
{
	int delay = 60 * 1000;
	while (true)
	{
		uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
		uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
		float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
		String mem_text = "free memory: " + String(percentageHeapFree) + " % free of " +
						  String(totalHeapBytes / 1000) + "k";
		String tasks_text = "currently running tasks: " + String(running_tasks);
		logprint(tasks_text);
		logprint(mem_text);
		mqtt_send("heartbeat", "hello");
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

// monitors & reconnects to wifi and mqtt
void network_task(void *parameter)
{
	int delay = 30 * 1000;
	while (true)
	{
		if (!wifi_connected())
		{
			wifi_init();
		}
		if (!mqtt_connected)
		{
			mqtt_init();
		}
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

// handles the auto switching of relays based on sensor data
void autopilot_task(void *parameter)
{
	int delay = 5 * 1000;
	while (true)
	{
		if (get_autopilot_state())
		{
			if (get_food_temp() < get_target_temp())
			{
				set_heater(true);
			}
			else
			{
				set_heater(false);
			}

			if (get_humidity() < get_target_hum())
			{
				set_humidifer(true);
			}
			else
			{
				set_humidifer(false);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

// updates the web interface
void web_update_task(void *parameter)
{
	int delay = 2 * 1000;
	while (true)
	{
		web_update();
		vTaskDelay(pdMS_TO_TICKS(delay));
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

void register_task(void (*taskFunction)(void *), String name)
{
	uint32_t defaut_stack_size = 20000;
	TaskHandle_t taskHandle = NULL;
	xTaskCreate(taskFunction, name.c_str(), defaut_stack_size, NULL, 1, &taskHandle);
	set_task_handler(taskHandle); // this is local variable, shouldn't this be a problem?
	logprint("registered task " + name);
}

void stop_all_tasks()
{
	for (int i = 0; i < running_tasks; i++)
	{
		if (task_handles[i] != NULL)
		{
			vTaskDelete(task_handles[i]);
			task_handles[i] = NULL;
		}
	}
}

void setup()
{
	Serial.begin(115200);
	nvm_init();
	control_init();
	wifi_init();
	mqtt_init();
	sensors_init();
	web_init();

	register_task(reporting_task, "reporting_task");
	register_task(network_task, "network_task");
	register_task(web_update_task, "web_update_task");
	register_task(autopilot_task, "autopilot_task");
}

void loop()
{
	// Empty, because we're using FreeRTOS tasks
}