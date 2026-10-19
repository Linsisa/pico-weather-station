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

/*! \brief This function is called when a stack overflow is detected in a FreeRTOS task.
 *
 * This function logs the stack overflow event and the name of the task that caused it.
 * It then enters an infinite loop, making the onboard LED blink rapidly to indicate a system crash.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // 1. Log the stack overflow event and the name of the task that caused it.
    printf("\n!!! CRITICAL STACK OVERFLOW DETECTED !!!\n");
    printf("The task that crashed the system is: [%s]\n", pcTaskName);

    // 2. Enter an infinite loop to indicate a system crash.
    const uint LED_PIN = CYW43_WL_GPIO_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(LED_PIN, LED_ON);
        sleep_ms(TASK_OVERFLOW_LED_BLINK_TIME_MS);
        gpio_put(LED_PIN, LED_OFF);
        sleep_ms(TASK_OVERFLOW_LED_BLINK_TIME_MS);
    }
}

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
