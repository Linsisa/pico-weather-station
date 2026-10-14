#ifndef TASK_COMM_H
#define TASK_COMM_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

extern QueueHandle_t sensor_data_queue;


void task_comm(void *pvParameters);

#endif // TASK_COMM_H
