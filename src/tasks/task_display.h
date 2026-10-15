#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

extern QueueHandle_t sensor_data_queue;

void task_display(void *pvParameters);

#endif // TASK_DISPLAY_H
