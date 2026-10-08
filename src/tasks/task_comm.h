#ifndef TASK_COMM_H
#define TASK_COMM_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

extern QueueHandle_t comm_sensor_data_queue; // Queue to hold sensor data for the communication task


void task_comm(void *pvParameters);

#endif // TASK_COMM_H
