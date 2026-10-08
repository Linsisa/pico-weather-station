#include "ldr_adc.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

/*!
 * \brief Initialize the LDR ADC.
 *
 * This function initializes the ADC and configures the GPIO pin and channel for the LDR from the configured settings.
 */
void ldr_adc_init(void) {
    adc_init();
    adc_gpio_init(LDR_ADC_PIN); // PIN for LDR is GPIO26, which corresponds to ADC0.
    adc_select_input(LDR_ADC_CHANNEL); // Select ADC input 0.
}

/*! 
 * \brief Read the LDR value from the ADC.
 *
 * This function reads the raw ADC value from the LDR and returns it as a 16-bit unsigned integer.
 *
 * \return The raw ADC value read from the LDR.
 */
uint16_t ldr_adc_read(void) {
    return adc_read();
}
