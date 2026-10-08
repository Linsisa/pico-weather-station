#include "dht11_pio.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "dht11.pio.h"

#include <stdio.h>

/*!
 * \brief Send the DHT11 start signal and release the bus for the sensor response.
 *
 * This function initializes the GPIO pin connected to the DHT11 sensor, sends
 * the start signal by pulling the line low for 20 milliseconds, and then releases
 * the line. After this, the pin is set to input mode to allow the DHT11 sensor to respond.
 */
static void dht11_send_start_signal(void) {
    /*
     * At each execution, this function uses gpio_init() to set the pin back to 
     * SIO control, as dht_program_init() switches it to PIO control.
     */
    gpio_init(DHT11_PIN);
    gpio_set_dir(DHT11_PIN, GPIO_OUT);

    gpio_put(DHT11_PIN, 0);
    sleep_ms(20);

    gpio_set_dir(DHT11_PIN, GPIO_IN);
}


/*! \brief  Read the 5 bytes received from the DHT11 through the PIO RX FIFO.
 *          Order: humidity integer, humidity decimal, temperature integer,
 *          temperature decimal, checksum.
 *
 * \param pio The PIO instance to use (e.g., pio0 or pio1).
 * \param bytes The array to store the received bytes.
 * \return true if the read was successful, false if there was a timeout.
 */
static bool dht11_read_bytes(PIO pio, uint8_t bytes[DHT_NUM_BYTES]) {
    uint64_t read_timeout = time_us_64() + DHT_TIMEOUT_US;

    for (uint8_t i = 0; i < DHT_NUM_BYTES; i++) {
        while (pio_sm_is_rx_fifo_empty(pio, DHT_SM)) {
            if (time_us_64() > read_timeout) {
                printf("DHT11 read timeout at byte %d\n", i);
                return false; // Timeout occurred, exit the function
            }
        }

        bytes[i] = pio_sm_get_blocking(pio, DHT_SM) & 0xFF;
    }
    return true;
}

/*! \brief Initialize the DHT11 PIO program.
 *
 * \return The offset of the initialized PIO program, or a negative value if initialization fails.
 */
int dht11_pio_program_init() {
    return pio_add_program(pio0, &dht_program);
}

/*! \brief Read data from the DHT11 sensor using the PIO program.
 *
 * \param program_offset The offset of the PIO program.
 * \param bytes The array to store the received bytes.
 * \return true if the read was successful, false if there was a timeout.
 */
bool dht11_read_data(int program_offset, uint8_t bytes[DHT_NUM_BYTES]) {
    dht11_send_start_signal();

    dht_program_init(pio0, DHT_SM, program_offset, DHT11_PIN, DHT_FREQ_HZ);

    return dht11_read_bytes(pio0, bytes);
}

/*! \brief Validate the checksum of the received DHT11 data.
 *
 * \param bytes The array containing the received bytes from the DHT11 sensor.
 * \return true if the checksum is valid, false otherwise.
 */
bool dht11_valid_checksum(const uint8_t bytes[DHT_NUM_BYTES]) {
    uint8_t checksum = bytes[0] + bytes[1] + bytes[2] + bytes[3];

    return checksum == bytes[4];
}
