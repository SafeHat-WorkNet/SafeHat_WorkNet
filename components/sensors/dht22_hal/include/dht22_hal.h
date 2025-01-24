/* components/sensors/dht22_hal/include/dht22_hal.h */

#ifndef MESHNET_DHT22_HAL_H
#define MESHNET_DHT22_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* Constants ******************************************************************/

extern const char    *dht22_tag;                    /**< Logging tag for ESP_LOG messages related to the DHT22 sensor. */
extern const uint8_t  dht22_data_io;                /**< GPIO pin number for the DHT22 data line. */
extern const uint32_t dht22_polling_rate_ticks;     /**< Polling interval for DHT22 in system ticks. */
extern const uint8_t  dht22_bit_count;              /**< Total number of bits transmitted by the DHT22 sensor (40 bits). */
extern const uint8_t  dht22_max_retries;            /**< Maximum retry attempts for DHT22 reinitialization. */
extern const uint32_t dht22_initial_retry_interval; /**< Initial retry interval for DHT22 in system ticks. */
extern const uint32_t dht22_max_backoff_interval;   /**< Maximum backoff interval for DHT22 retries in system ticks. */
extern const uint32_t dht22_start_delay_ms;         /**< Start signal delay for DHT22 in milliseconds. */
extern const uint32_t dht22_response_timeout_us;    /**< Timeout for DHT22 sensor response in microseconds. */
extern const uint32_t dht22_bit_threshold_us;       /**< Timing threshold for distinguishing bits in DHT22 signal. */
extern const uint8_t  dht22_allowed_fail_attempts;  /**< Number of allowed consecutive failures */

/* Enums **********************************************************************/

/**
 * @brief Enumeration of DHT22 sensor states.
 *
 * Represents the possible operational states of the DHT22 temperature and humidity sensor,
 * including normal operation, data updates, initialization status, and error conditions.
 */
typedef enum : uint8_t {
  k_dht22_ready         = 0x00, /**< Sensor is initialized and ready to provide data. */
  k_dht22_data_updated  = 0x01, /**< New data is available from the sensor. */
  k_dht22_uninitialized = 0x10, /**< Sensor has not been initialized. */
  k_dht22_error         = 0xF0, /**< General catch-all error state. */
} dht22_states_t;

/* Structs ********************************************************************/

/**
 * @brief Structure for managing DHT22 sensor data and state.
 *
 * Contains the latest temperature and humidity readings from the DHT22 sensor,
 * as well as state and retry management variables for error handling and reinitialization.
 */
typedef struct {
  float      temperature_f;      /**< Latest temperature reading in Fahrenheit. */
  float      temperature_c;      /**< Latest temperature reading in Celsius. */
  float      humidity;           /**< Latest humidity reading as a percentage. */
  uint8_t    state;              /**< Current operational state of the sensor (see dht22_states_t). */
  uint8_t    retry_count;        /**< Number of consecutive reinitialization attempts. */
  uint32_t   retry_interval;     /**< Current interval between reinitialization attempts, in ticks. */
  TickType_t last_attempt_ticks; /**< Tick count of the last reinitialization attempt. */
  uint8_t    fail_count;         /**< Number of current amount of fail reads */
} dht22_data_t;

/* Public Functions ***********************************************************/

/**
 * @brief Converts DHT22 sensor data to a JSON string.
 *
 * Converts the temperature and humidity data from a `dht22_data_t` structure 
 * into a dynamically allocated JSON-formatted string. The caller is responsible 
 * for freeing the memory.
 *
 * @param[in] sensor_data Pointer to the `dht22_data_t` structure containing 
 *                        sensor data.
 *
 * @return 
 * - Pointer to the JSON-formatted string on success.
 * - `NULL` if memory allocation fails.
 */
char *dht22_data_to_json(const dht22_data_t *data);

/**
 * @brief Initializes the DHT22 sensor for temperature and humidity measurements.
 *
 * Configures the GPIO pin connected to the DHT22 data line and prepares the 
 * `dht22_data_t` structure to store sensor readings. Updates the sensor state 
 * to indicate readiness for data acquisition.
 *
 * @param[in,out] sensor_data Pointer to the `dht22_data_t` structure for storing
 *                            temperature, humidity, and state information.
 *
 * @return 
 * - `ESP_OK` on successful initialization.
 * - Relevant `esp_err_t` code on failure.
 *
 * @note 
 * - Call this function before reading data from the sensor.
 * - Ensures the sensor is in a ready state (`k_dht22_ready`) upon success.
 */
esp_err_t dht22_init(void *sensor_data);

/**
 * @brief Reads temperature and humidity data from the DHT22 sensor.
 *
 * Retrieves temperature and humidity readings from the DHT22 sensor. Updates 
 * the `dht22_data_t` structure with the new data or sets the state to indicate 
 * an error if the read operation fails.
 *
 * @param[in,out] sensor_data Pointer to the `dht22_data_t` structure to store
 *                            temperature, humidity, and state information.
 *
 * @return 
 * - `ESP_OK`   on successful read.
 * - `ESP_FAIL` on unsuccessful read.
 *
 * @note 
 * - Call `dht22_init` before using this function.
 */
esp_err_t dht22_read(dht22_data_t *sensor_data);

/**
 * @brief Handles error recovery for the DHT22 sensor using exponential backoff.
 *
 * Attempts to reinitialize the DHT22 sensor when an error is detected. Uses an 
 * exponential backoff strategy to manage retry intervals and reduce system load 
 * during repeated failures. Resets retry logic on successful recovery.
 *
 * @param[in,out] sensor_data Pointer to the `dht22_data_t` structure managing 
 *                            sensor state and retries.
 *
 * @note 
 * - Call this function periodically within the sensor task to handle errors.
 */
void dht22_reset_on_error(dht22_data_t *sensor_data);

/**
 * @brief Periodically reads data from the DHT22 sensor and manages errors.
 *
 * Designed to run within a FreeRTOS task, this function continuously reads data 
 * from the DHT22 sensor at intervals defined by `dht22_polling_rate_ticks`. It 
 * handles errors by invoking `dht22_reset_on_error` with an exponential backoff strategy.
 *
 * @param[in,out] sensor_data Pointer to the `dht22_data_t` structure for managing 
 *                            sensor data and state.
 *
 * @note 
 * - Execute this function as part of a FreeRTOS task for continuous operation.
 * - Includes error handling to ensure reliable data acquisition.
 */
void dht22_tasks(void *sensor_data);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_DHT22_HAL_H */
