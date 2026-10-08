#ifndef TASK_SENSORS_H
#define TASK_SENSORS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

extern QueueHandle_t display_sensor_data_queue; // Queue to hold sensor data for the display task.
extern QueueHandle_t comm_sensor_data_queue; // Queue to hold sensor data for the communication task.
extern TaskHandle_t display_task_handle; // Handle for the display task.

void task_sensors(void *pvParameters);

#endif // TASK_SENSORS_H
