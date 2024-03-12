#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "utils.h"
#include "control.h"
#include "nvm.h"
#include "network.h"

// prints available memory
void memory_task(void *parameter) {
	int delay = 10 *1000;
	while (true) {
		uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
		uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
		float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
		String text = "free memory: " + String(percentageHeapFree) + " % free of " + \
					String(totalHeapBytes / 1000) + "k";
		logprint(text);
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

// monitors & reconnects to wifi and mqtt
void network_task(void *parameter) {
	int delay = 60 * 1000;
	while (true) {
		if (!wifi_connected()) {
			wifi_init();
		}
		if (!mqtt_connected) {
			mqtt_init();
		}
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

void setup() {
	Serial.begin(115200);
	control_init();
	nvm_init();
	wifi_init();
	mqtt_init();

	xTaskCreate(memory_task, "memory_task", 10000, NULL, 1, NULL);
	xTaskCreate(network_task, "network_task", 10000, NULL, 1, NULL);
}

void loop() {
	// Empty, because we're using FreeRTOS tasks
}
