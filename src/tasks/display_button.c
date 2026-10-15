#include "display_button.h"

#include "config.h"

#include "pico/stdlib.h"

#include <stdint.h>


static volatile uint64_t last_display_change_button_press_time = 0; // Variable to track the last button press time.


/*! \brief Callback function for the display button press event.
 *
 * This function is called when the display button is pressed.
 * It tracks the time of the button press to avoid multiple rapid presses 
 * and notifies the display task that a screen mode change is requested.
 *
 * \param gpio The GPIO pin number that triggered the interrupt.
 * \param events The event type that triggered the interrupt (e.g., rising edge).
 */
static void display_button_callback(uint gpio, uint32_t events) {
    // This function is called when the display button is pressed and notify the display task that screen mode change is requested.
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_RISE)) {
        uint64_t current_time = time_us_64();

        if (current_time - last_display_change_button_press_time >= BUTTON_COOLDOWN_US) {
            last_display_change_button_press_time = current_time;

            // Notify the display task that a screen mode change is requested.
            if (display_task_handle != NULL) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                
                xTaskNotifyFromISR(display_task_handle, BUTTON_NOTIFY_BIT, eSetBits, &xHigherPriorityTaskWoken);

                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}

/*! \brief Initialize the display mode button GPIO and set up the interrupt.
 *
 * This function configures the GPIO pin for the display mode button as an input,
 * enables the pull-down resistor, and sets up an interrupt to trigger on a rising edge.
 * The interrupt will call the display_button_callback function when the button is pressed.
 */
void display_button_init(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); 
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &display_button_callback);
}
