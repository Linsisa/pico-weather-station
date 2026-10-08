#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

extern QueueHandle_t display_sensor_data_queue; // Queue to hold sensor data for the display task
extern TaskHandle_t display_task_handle; // Handle for the display task

void task_display(void *pvParameters);

#endif // TASK_DISPLAY_H
