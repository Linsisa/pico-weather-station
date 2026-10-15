#ifndef DISPLAY_BUTTON_H
#define DISPLAY_BUTTON_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t display_task_handle; // Handle for the display task

void display_button_init(void);

#endif // DISPLAY_BUTTON_H
