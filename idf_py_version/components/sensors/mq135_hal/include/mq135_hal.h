/* components/sensors/mq135_hal/include/mq135_hal.h */

#ifndef SAFEHAT_WORKNET_MQ135_HAL_H
#define SAFEHAT_WORKNET_MQ135_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "error_handler.h"

/* Constants ******************************************************************/

extern const char    *mq135_tag;                    /**< Tag for ESP_LOG messages related to the MQ135 sensor. */
extern const uint8_t  mq135_aout_pin;               /**< GPIO pin for analog output (AOUT) of the MQ135 sensor. */
extern const uint8_t  mq135_dout_pin;               /**< GPIO pin for digital output (DOUT) of the MQ135 sensor. */
extern const uint32_t mq135_polling_rate_ticks;     /**< Polling rate for MQ135 sensor reads in system ticks. */
extern const uint32_t mq135_warmup_time_ms;         /**< Warm-up time for MQ135 sensor in milliseconds. */
extern const uint8_t  mq135_max_retries;            /**< Maximum retry attempts for MQ135 error recovery. */
extern const uint32_t mq135_initial_retry_interval; /**< Initial retry interval for MQ135 error recovery in ticks. */
extern const uint32_t mq135_max_backoff_interval;   /**< Maximum backoff interval for MQ135 retries in ticks. */
extern const uint8_t  mq135_allowed_fail_attempts;  /**< Number of allowed consecutive failures before reset. */

/* Enums **********************************************************************/

/**
 * @brief Enumeration of MQ135 sensor states.
 *
 * Defines the possible states of the MQ135 air quality sensor for tracking its operational
 * status, error handling, and data acquisition tasks.
 */
typedef enum : uint8_t {
  k_mq135_ready      = 0x00, /**< Sensor is initialized and ready to read data. */
  k_mq135_warming_up = 0x01, /**< Sensor is stabilizing during the warm-up period. */
  k_mq135_error      = 0xF0, /**< General error state. */
  k_mq135_read_error = 0xA1, /**< Error occurred while reading the analog output. */
} mq135_states_t;

/* Structs ********************************************************************/

/**
 * @brief Structure to store MQ135 sensor data and status.
 *
 * Holds data and status information for the MQ135 air quality sensor, including
 * the raw ADC reading, calculated gas concentration in ppm, warm-up timing, and
 * error handling through the error_handler_t structure.
 */
typedef struct {
  uint16_t         raw_adc_value;      /**< Raw ADC value read from the analog output of the sensor. */
  float            gas_concentration;   /**< Calculated gas concentration in parts per million (ppm). */
  uint8_t          state;              /**< Current operational state of the sensor (see `mq135_states_t`). */
  TickType_t       warmup_start_ticks; /**< Tick count when the warm-up period started. */
  error_handler_t  error_handler;      /**< Error handler for managing sensor errors and recovery. */
} mq135_data_t;

/* Public Functions ***********************************************************/

/**
 * @brief Converts MQ135 sensor data to a JSON string.
 *
 * Converts the gas concentration data in a `mq135_data_t` structure to a 
 * dynamically allocated JSON string. The caller must free the memory.
 *
 * @param[in] data Pointer to the `mq135_data_t` structure with valid sensor data.
 *
 * @return 
 * - Pointer to the JSON-formatted string on success.
 * - `NULL` if memory allocation fails.
 */
char *mq135_data_to_json(const mq135_data_t *data);

/**
 * @brief Initializes the MQ135 sensor.
 *
 * Configures the GPIO and ADC settings for the MQ135 sensor and starts the
 * warm-up period. Also initializes the error handler for robust error recovery.
 *
 * @param[in,out] sensor_data Pointer to the `mq135_data_t` structure holding
 *                            initialization parameters and state.
 *
 * @return 
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` codes on failure.
 *
 * @note 
 * - Call this function during system initialization.
 */
esp_err_t mq135_init(void *sensor_data);

/**
 * @brief Reads gas concentration data from the MQ135 sensor.
 *
 * Reads the raw analog value from the sensor's AOUT pin, calculates the gas
 * concentration in ppm, and updates the `mq135_data_t` structure.
 *
 * @param[in,out] sensor_data Pointer to the `mq135_data_t` structure to store
 *                            the sensor data and read status.
 *
 * @return 
 * - `ESP_OK`   on successful read.
 * - `ESP_FAIL` on failure.
 *
 * @note
 * - Ensure the sensor is initialized with `mq135_init` before calling.
 */
esp_err_t mq135_read(mq135_data_t *sensor_data);

/**
 * @brief Executes periodic tasks for the MQ135 sensor.
 *
 * Periodically reads data and handles errors for the MQ135 sensor using
 * the error handler for recovery. Intended to run in a FreeRTOS task.
 *
 * @param[in,out] sensor_data Pointer to the `mq135_data_t` structure for managing
 *                            sensor data and error recovery.
 *
 * @note 
 * - Should run at intervals defined by `mq135_polling_rate_ticks`.
 * - Uses error_handler_t for error recovery to maintain stable operation.
 */
void mq135_tasks(void *sensor_data);

#ifdef __cplusplus
}
#endif

#endif /* SAFEHAT_WORKNET_MQ135_HAL_H */


