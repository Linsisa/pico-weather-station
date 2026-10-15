#include "tasks/task_display.h"
#include "tasks/task_sensors.h"
#include "tasks/task_comm.h"
#include "config.h"
#include "sensor_data.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>


QueueHandle_t sensor_data_queue = NULL; // Queue for sensor data used by the display and communication tasks.
TaskHandle_t display_task_handle = NULL; // Handle for the display task.


int main() {
    stdio_init_all();

    sensor_data_queue = xQueueCreate(1, sizeof(sensor_data_t));

    if (sensor_data_queue != NULL) {
        BaseType_t sensors_task = xTaskCreate(task_sensors, "Sensors", SENSOR_TASK_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY, NULL);
        BaseType_t display_task = xTaskCreate(task_display, "Display", DISPLAY_TASK_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &display_task_handle);
        BaseType_t comm_task = xTaskCreate(task_comm, "Wi-Fi Comm", COMM_TASK_STACK_SIZE, NULL, COMM_TASK_PRIORITY, NULL);

        if (sensors_task != pdPASS || 
            display_task != pdPASS || 
            comm_task != pdPASS) {
            printf("Failed to create tasks.\n");
            return 1;
        }

        vTaskStartScheduler();
    } 
    else { printf("Failed to create queues.\n"); }

    while (true) {}
}
