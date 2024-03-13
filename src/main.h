#ifndef MAIN
#define MAIN

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"

TaskHandle_t task_handles[MAX_TASK_HANDLES] = {NULL};
int running_tasks = 0;

void stop_all_tasks();

#endif /* MAIN */
