/* components/sensors/dht22_hal/dht22_hal.c */

#include "dht22_hal.h"
#include <stdio.h>
#include <string.h>
#include "file_write_manager.h"
#include "webserver_tasks.h"
#include "cJSON.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "error_handler.h"

/* Constants *******************************************************************/

const char    *dht22_tag                    = "DHT22";
const uint8_t  dht22_data_io                = GPIO_NUM_4;
const uint32_t dht22_polling_rate_ticks     = pdMS_TO_TICKS(5 * 1000);
const uint8_t  dht22_bit_count              = 40;
const uint8_t  dht22_max_retries            = 4;
const uint32_t dht22_initial_retry_interval = pdMS_TO_TICKS(15 * 1000);
const uint32_t dht22_max_backoff_interval   = pdMS_TO_TICKS(480 * 1000);
const uint32_t dht22_start_delay_ms         = 20;
const uint32_t dht22_response_timeout_us    = 80;
const uint32_t dht22_bit_threshold_us       = 40;
const uint8_t  dht22_allowed_fail_attempts  = 3;

/* Globals (Static) ***********************************************************/

static error_handler_t s_dht22_error_handler = { 0 };

/* Static (Private) Functions **************************************************/

/**
 * @brief Initializes the GPIO pin connected to the DHT22 sensor.
 *
 * Configures the specified GPIO pin as an output with a pull-up resistor enabled
 * and sets the pin to an initial high state for communication readiness.
 *
 * @param[in] data_io GPIO pin number connected to the DHT22 data line.
 *
 * @return 
 * - `ESP_OK` on successful configuration.
 * - Relevant `esp_err_t` code on failure.
 *
 * @note 
 * - This function should be called during sensor initialization.
 */
static esp_err_t priv_dht22_gpio_init(uint8_t data_io)
{
  gpio_config_t io_conf;
  io_conf.pin_bit_mask = (1ULL << data_io);
  io_conf.mode         = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.intr_type    = GPIO_INTR_DISABLE;

  return gpio_config(&io_conf);
}

/**
 * @brief Waits for a specific GPIO level and measures the duration.
 *
 * Monitors a GPIO pin connected to the DHT22 sensor, waiting for it to reach 
 * the specified level (`0` or `1`). Measures the time taken to reach this level 
 * and stores the duration. Returns `false` if the level is not reached within 
 * the given timeout.
 *
 * @param[in]  level       Desired GPIO level (0 or 1).
 * @param[in]  timeout_us  Maximum time to wait for the level, in microseconds.
 * @param[out] duration_us Pointer to store the time taken to reach the level, 
 *                         in microseconds.
 *
 * @return 
 * - `true`  if the level was reached within the timeout.
 * - `false` if the timeout occurred.
 */
static bool priv_dht22_wait_for_level_with_duration(int8_t level, uint32_t timeout_us,
                                                    uint32_t *duration_us)
{
  uint64_t start_time = esp_timer_get_time();
  while (gpio_get_level(dht22_data_io) != level) {
    if ((esp_timer_get_time() - start_time) >= timeout_us) {
      return false; /* Timeout occurred */
    }
  }
  *duration_us = (uint32_t)(esp_timer_get_time() - start_time);
  return true; /* Success */
}

/**
 * @brief Sends the start signal to the DHT22 sensor.
 *
 * Pulls the data line low for a specified duration (`dht22_start_delay_ms`) 
 * to signal readiness for communication. After the delay, the line is released 
 * by setting it high, followed by a short delay before switching the GPIO mode 
 * back to input. This initiates communication with the DHT22 sensor.
 *
 * @note 
 * - Ensure the GPIO pin is properly configured before calling this function.
 * - The `dht22_start_delay_ms` constant must be defined.
 */
static void priv_dht22_send_start_signal(void)
{
  gpio_set_direction(dht22_data_io, GPIO_MODE_OUTPUT);
  gpio_set_level(dht22_data_io, 0); /* Pull line low */
  esp_rom_delay_us(dht22_start_delay_ms * 1000); /* Wait for start signal duration */

  gpio_set_level(dht22_data_io, 1); /* Release line */
  esp_rom_delay_us(30); /* Wait for 30 microseconds before switching to input */
}

/**
 * @brief Waits for the DHT22 sensor to respond after the start signal.
 *
 * Monitors the GPIO line for the DHT22 response sequence: 
 * - Line goes low (~80 µs) indicating the start of the response.
 * - Line goes high (~80 µs) indicating continuation.
 * - Line goes low again, signaling readiness to transmit data.
 *
 * If the expected transitions do not occur within the timeout, the function returns `ESP_FAIL`.
 *
 * @return 
 * - `ESP_OK`   if the sensor responds with the expected sequence.
 * - `ESP_FAIL` if the sensor fails to respond within the timeout.
 */
static esp_err_t priv_dht22_wait_for_response(void)
{
  uint32_t duration = 0;

  /* DHT22 pulls the line low for 80us, then high for 80us, as a response signal */
  if (!priv_dht22_wait_for_level_with_duration(0, dht22_response_timeout_us, &duration) ||
      !priv_dht22_wait_for_level_with_duration(1, dht22_response_timeout_us, &duration) ||
      !priv_dht22_wait_for_level_with_duration(0, dht22_response_timeout_us, &duration)) {
    ESP_LOGE(dht22_tag, "Sensor not responding");
    return ESP_FAIL;
  }
  return ESP_OK;
}

/**
 * @brief Reads a single bit of data from the DHT22 sensor.
 *
 * Measures the duration of a high-level pulse from the DHT22 to determine 
 * if the bit is `0` or `1`. A short pulse represents `0`, while a longer pulse 
 * represents `1`.
 *
 * @return 
 * - `0`        if the bit is `0`.
 * - `1`        if the bit is `1`.
 * - `ESP_FAIL` if the bit cannot be read within the expected timing constraints.
 */
static int8_t priv_dht22_read_bit(void)
{
  uint32_t duration = 0;

  /* Wait for the line to go high (start of bit transmission) */
  if (!priv_dht22_wait_for_level_with_duration(1, 50, &duration)) {
    ESP_LOGE(dht22_tag, "Timeout waiting for bit start");
    return ESP_FAIL;
  }

  /* Wait for the line to go low again, measuring the duration of the high level */
  if (!priv_dht22_wait_for_level_with_duration(0, 70, &duration)) {
    ESP_LOGE(dht22_tag, "Timeout waiting for bit end");
    return ESP_FAIL;
  }

  /* Determine if the bit is '0' or '1' based on the duration */
  if (duration > dht22_bit_threshold_us) {
    return 1; /* Bit is '1' */
  } else {
    return 0; /* Bit is '0' */
  }
}

/**
 * @brief Reads the full 40 bits (5 bytes) of data from the sensor.
 *
 * The DHT22 sensor transmits 40 bits of data, which include humidity, temperature, 
 * and a checksum. This function reads each bit from the sensor and assembles them 
 * into 5 bytes stored in the provided data buffer. If any bit cannot be read 
 * within the expected timing constraints, the function will return an error.
 *
 * @param[out] data_buffer Array to store the 40 bits (5 bytes) of data.
 *
 * @return
 * - `ESP_OK`   on successful data read.
 * - `ESP_FAIL` on timeout or other errors while reading bits.
 */
static esp_err_t priv_dht22_read_data_bits(uint8_t *data_buffer)
{
  uint8_t byte_index = 0, bit_index = 7;
  memset(data_buffer, 0, 5);
  for (uint8_t i = 0; i < dht22_bit_count; i++) {
    int8_t bit = priv_dht22_read_bit();
    if (bit == ESP_FAIL) {
      return ESP_FAIL;
    }
    if (bit == 1) {
      data_buffer[byte_index] |= (1 << bit_index); /* Set bit to '1' */
    }

    /* Move to the next bit */
    if (bit_index == 0) {
      bit_index = 7;
      byte_index++;
    } else {
      bit_index--;
    }
  }

  return ESP_OK;
}

/**
 * @brief Verifies the checksum of the data received from the DHT22.
 *
 * The DHT22 sensor sends 5 bytes of data, with the first four bytes representing 
 * humidity and temperature, and the fifth byte being a checksum. This function 
 * calculates the checksum from the first four bytes and compares it to the received 
 * checksum byte to ensure data integrity. If the checksum matches, the function 
 * returns `ESP_OK`; otherwise, it returns an error.
 *
 * @param[in] data_buffer Buffer containing the 40 bits (5 bytes) of data received 
 *                        from the sensor.
 *
 * @return
 * - `ESP_OK`   if the calculated checksum matches the received checksum.
 * - `ESP_FAIL` if the checksum verification fails.
 */
static esp_err_t priv_dht22_verify_checksum(uint8_t *data_buffer)
{
  uint8_t checksum = data_buffer[0] + data_buffer[1] + data_buffer[2] + data_buffer[3];
  if ((checksum & 0xFF) != data_buffer[4]) {
    ESP_LOGE(dht22_tag, "Checksum failed");
    return ESP_FAIL;
  }
  return ESP_OK;
}

/* Public Functions ************************************************************/

char *dht22_data_to_json(const dht22_data_t *data)
{
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    ESP_LOGE(dht22_tag, "Failed to create JSON object.");
    return NULL;
  }

  if (!cJSON_AddStringToObject(json, "sensor_type", "temperature_humidity")) {
    ESP_LOGE(dht22_tag, "Failed to add sensor_type to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "temperature_c", data->temperature_c)) {
    ESP_LOGE(dht22_tag, "Failed to add temperature_c to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "temperature_f", data->temperature_f)) {
    ESP_LOGE(dht22_tag, "Failed to add temperature_f  to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "humidity", data->humidity)) {
    ESP_LOGE(dht22_tag, "Failed to add humidity to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  char *json_string = cJSON_PrintUnformatted(json);
  if (!json_string) {
    ESP_LOGE(dht22_tag, "Failed to serialize JSON object.");
    cJSON_Delete(json);
    return NULL;
  }

  cJSON_Delete(json);
  return json_string;
}

esp_err_t dht22_init(void *sensor_data)
{
  dht22_data_t *dht22_data = (dht22_data_t *)sensor_data;
  ESP_LOGI(dht22_tag, "Starting Configuration");

  /* TODO: Initialize error handler */

  dht22_data->humidity           = -1.0;
  dht22_data->temperature_f      = -1.0;
  dht22_data->temperature_c      = -1.0;
  dht22_data->state              = k_dht22_uninitialized; /* Start uninitialized */
  dht22_data->retry_count        = 0;
  dht22_data->retry_interval     = dht22_initial_retry_interval;
  dht22_data->last_attempt_ticks = 0;
  dht22_data->fail_count         = 0;

  esp_err_t ret = priv_dht22_gpio_init(dht22_data_io);
  if (ret != ESP_OK) {
    ESP_LOGE(dht22_tag, "Failed to configure GPIO: %s", esp_err_to_name(ret));
    return ret;
  }

  gpio_set_level(dht22_data_io, 1);
  dht22_data->state = k_dht22_ready;
  ESP_LOGI(dht22_tag, "Sensor Configuration Complete");
  return ESP_OK;
}

esp_err_t dht22_read(dht22_data_t *sensor_data)
{
  if (sensor_data == NULL) {
    ESP_LOGE(dht22_tag, "Sensor data pointer is NULL");
    return ESP_FAIL;
  }

  uint8_t   data_buffer[5] = {0};
  esp_err_t ret;

  /* Send start signal and wait for response */
  priv_dht22_send_start_signal();
  gpio_set_direction(dht22_data_io, GPIO_MODE_INPUT);

  ret = priv_dht22_wait_for_response();
  if (ret != ESP_OK) {
    sensor_data->fail_count++;
    sensor_data->state = k_dht22_error;
    ESP_LOGE(dht22_tag, "Failed to receive response from DHT22");
    return ESP_FAIL;
  }

  /* Read data bits */
  ret = priv_dht22_read_data_bits(data_buffer);
  if (ret != ESP_OK) {
    sensor_data->fail_count++;
    sensor_data->state = k_dht22_error;
    ESP_LOGE(dht22_tag, "Failed to read data bits from DHT22");
    return ESP_FAIL;
  }

  /* Verify checksum */
  ret = priv_dht22_verify_checksum(data_buffer);
  if (ret != ESP_OK) {
    sensor_data->fail_count++;
    sensor_data->state = k_dht22_error;
    ESP_LOGE(dht22_tag, "Checksum verification failed");
    return ESP_FAIL;
  }

  sensor_data->fail_count = 0; /* Reset count on successful read */

  /* Convert raw data to meaningful values */
  sensor_data->humidity      = ((data_buffer[0] << 8) + data_buffer[1]) * 0.1f;
  sensor_data->temperature_c = (((data_buffer[2] & 0x7F) << 8) + data_buffer[3]) * 0.1f;
  sensor_data->temperature_f = sensor_data->temperature_c * 9.0 / 5.0 + 32; /* Convert Celsius to Fahrenheit */

  /* Handle negative temperatures */
  if (data_buffer[2] & 0x80) {
    sensor_data->temperature_c = -sensor_data->temperature_c;
    sensor_data->temperature_f = -sensor_data->temperature_f;
  }

  sensor_data->state = k_dht22_data_updated;

  ESP_LOGI(dht22_tag,
           "Temperature: %.1f F (%.1f C), Humidity: %.1f %%",
           sensor_data->temperature_f,
           sensor_data->temperature_c,
           sensor_data->humidity);
  return ESP_OK;
}

void dht22_reset_on_error(dht22_data_t *sensor_data)
{
  if (sensor_data->fail_count >= dht22_allowed_fail_attempts) {
    /* TODO: Reset error handler */
  }
}

void dht22_tasks(void *sensor_data)
{
  dht22_data_t *dht22_data = (dht22_data_t *)sensor_data;
  while (1) {
    if (dht22_read(dht22_data) == ESP_OK) {
      char *json = dht22_data_to_json(dht22_data);
      send_sensor_data_to_webserver(json);
      file_write_enqueue("dht22.txt", json);
      free(json);
    } else {
      dht22_reset_on_error(dht22_data);
    }
    vTaskDelay(dht22_polling_rate_ticks);
  }
}
