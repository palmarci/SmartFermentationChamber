#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Task 1 function
void task1(void *parameter) {
	while (true) {
		Serial.println("Task 1 is running");
		vTaskDelay(pdMS_TO_TICKS(1000)); // Delay of 1 second
	}
}

// Task 2 function
void task2(void *parameter) {
	while (true) {
		Serial.println("Task 2 is running");
		vTaskDelay(pdMS_TO_TICKS(2000)); // Delay of 2 seconds
	}
}

void setup() {
	Serial.begin(115200);

	// Create Task 1
	xTaskCreate(task1, "Task 1", 10000, NULL, 1, NULL);

	// Create Task 2
	xTaskCreate(task2, "Task 2", 10000, NULL, 1, NULL);
}

void loop() {
	// Empty, because we're using FreeRTOS tasks
}
