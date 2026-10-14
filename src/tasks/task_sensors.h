#ifndef TASK_SENSORS_H
#define TASK_SENSORS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

extern QueueHandle_t sensor_data_queue; // Queue for sensor data used by the display and communication tasks.
extern TaskHandle_t display_task_handle; // Handle for the display task.

void task_sensors(void *pvParameters);

#endif // TASK_SENSORS_H
