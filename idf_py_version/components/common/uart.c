/* components/common/uart.c */

#include "common/uart.h"
#include "driver/uart.h"
#include "esp_log.h"

/* Constants ******************************************************************/

const uint32_t uart_timeout_ticks = pdMS_TO_TICKS(1000); /* Timeout for UART operations in ticks */

/* Private Functions **********************************************************/

esp_err_t priv_uart_init(uint8_t tx_io, uint8_t rx_io, uint32_t baud_rate,
                         uart_port_t uart_num, const char *tag)
{
  uart_config_t uart_config = {
    .baud_rate = baud_rate,                /* Set the baud rate (communication speed) */
    .data_bits = UART_DATA_8_BITS,         /* Set data bits (8 bits per word) */
    .parity    = UART_PARITY_DISABLE,      /* No parity check */
    .stop_bits = UART_STOP_BITS_1,         /* Set 1 stop bit */
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, /* Disable hardware flow control */
  };

  /* Configure the UART driver with the specified settings */
  esp_err_t ret = uart_param_config(uart_num, &uart_config);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "UART parameter configuration failed: %s", esp_err_to_name(ret));
    return ret;
  }

  /* Set the TX and RX pin numbers for the UART interface */
  ret = uart_set_pin(uart_num, tx_io, rx_io, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "UART pin configuration failed: %s", esp_err_to_name(ret));
    return ret;
  }

  /* Install the UART driver with no buffer (0 TX and RX buffer size) */
  ret = uart_driver_install(uart_num, 1024 * 2, 0, 0, NULL, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(tag, "UART driver installation failed: %s", esp_err_to_name(ret));
  }

  return ret; /* Return the error status or ESP_OK */
}

esp_err_t priv_uart_read(uint8_t *data, size_t len, int32_t *out_length,
                         uart_port_t uart_num, const char *tag)
{
  /* Read UART data from the specified UART port */
  int32_t length = uart_read_bytes(uart_num, data, len, uart_timeout_ticks);

  if (length > 0) {
    *out_length = length; /* Store the length of data read */
    ESP_LOGI(tag, "Received UART data: %.*s", (int)length, data); /* Log the received data */
    return ESP_OK;
  } else {
    ESP_LOGE(tag, "UART read failed or timed out");
    *out_length = 0; /* No data read */
    return ESP_FAIL;
  }
}
