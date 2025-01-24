/* components/common/include/common/uart.h */

#ifndef MESHNET_UART_H
#define MESHNET_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"

/* Constants ******************************************************************/

extern const uint32_t uart_timeout_ticks; /**< Timeout for UART commands in ticks */

/* Private Functions **********************************************************/

/**
 * @brief Initializes the UART interface with specified parameters.
 *
 * Configures the UART interface with the specified TX and RX pins, baud rate,
 * and UART port number. Sets the UART communication parameters such as baud rate,
 * data bits, parity, and stop bits. The UART driver is initialized with no buffer
 * allocation (TX and RX buffers set to 0).
 *
 * @param[in] tx_io        Pin number for the TX (transmit) line.
 * @param[in] rx_io        Pin number for the RX (receive) line.
 * @param[in] baud_rate    Communication baud rate (e.g., 9600, 115200).
 * @param[in,out] uart_num UART port number to configure.
 * @param[in] tag          Tag for logging errors and events.
 *
 * @return 
 * - `ESP_OK` on successful initialization.
 * - Error codes from `esp_err_t` on failure (e.g., invalid arguments or driver errors).
 *
 * @note 
 * - Ensure the specified pins are free and not used by other peripherals.
 * - This function assumes the UART driver is not already initialized on the given port.
 */
esp_err_t priv_uart_init(uint8_t tx_io, uint8_t rx_io, uint32_t baud_rate,
                         uart_port_t uart_num, const char *tag);

/**
 * @brief Reads data from the UART interface and returns the length of data read.
 *
 * Reads up to `len` bytes from the specified UART interface. Uses a timeout
 * mechanism to wait for data. On success, the read data is stored in `data`, 
 * and the number of bytes read is saved in `out_length`.
 *
 * @param[out] data       Buffer to store the read data.
 * @param[in] len         Maximum number of bytes to read.
 * @param[out] out_length Pointer to an integer to store the number of bytes read.
 * @param[in] uart_num    UART port number to read from.
 * @param[in] tag         Tag for logging errors and events.
 *
 * @return 
 * - `ESP_OK`   if data is successfully read.
 * - `ESP_FAIL` if the read operation fails or times out.
 *
 * @note 
 * - Ensure the UART interface is initialized using `priv_uart_init` before calling this function.
 * - The `data` buffer must have sufficient space for `len` bytes.
 * - This function does not implement retry logic; consider adding retries if necessary.
 */
esp_err_t priv_uart_read(uint8_t *data, size_t len, int32_t *out_length,
                         uart_port_t uart_num, const char *tag);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_UART_H */

