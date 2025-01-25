/* components/sensors/bh1750_hal/include/bh1750_hal.h */

#ifndef SAFEHAT_WORKNET_BH1750_HAL_H
#define SAFEHAT_WORKNET_BH1750_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

/* Constants ******************************************************************/

extern const uint8_t    bh1750_i2c_address;            /**< I2C address of the BH1750 sensor (default 0x23 when ADDR pin is GND). */
extern const i2c_port_t bh1750_i2c_bus;                /**< I2C bus number used by the ESP32 to communicate with the BH1750 sensor. */
extern const char      *bh1750_tag;                    /**< Tag for ESP_LOG messages related to the BH1750 sensor. */
extern const uint8_t    bh1750_scl_io;                 /**< GPIO pin for the I2C Serial Clock Line (SCL). */
extern const uint8_t    bh1750_sda_io;                 /**< GPIO pin for the I2C Serial Data Line (SDA). */
extern const uint32_t   bh1750_i2c_freq_hz;            /**< I2C bus frequency in Hz for BH1750 communication (default 100 kHz). */
extern const uint32_t   bh1750_polling_rate_ticks;     /**< Polling rate for the BH1750 sensor in system ticks. */
extern const uint8_t    bh1750_max_retries;            /**< Maximum retry attempts for BH1750 sensor reinitialization. */
extern const uint32_t   bh1750_initial_retry_interval; /**< Initial retry interval in ticks for BH1750 reinitialization. */
extern const uint32_t   bh1750_max_backoff_interval;   /**< Maximum backoff interval in ticks for BH1750 reinitialization retries. */

/* Enums **********************************************************************/

/**
 * @brief I2C commands for the BH1750 sensor.
 *
 * Defines the available I2C commands for controlling the BH1750 light intensity sensor.
 * These commands include power management, measurement mode selection, and timing adjustments.
 */
typedef enum : uint8_t {
  k_bh1750_power_down_cmd                = 0x00, /**< Puts the sensor into a power-down state to save energy. */
  k_bh1750_power_on_cmd                  = 0x01, /**< Powers on the sensor, enabling measurements. */
  k_bh1750_reset_cmd                     = 0x07, /**< Resets the data register without changing the measurement mode. */
  k_bh1750_cont_low_res_mode_cmd         = 0x13, /**< Sets the sensor to continuously measure in low-resolution mode (4 lx/bit). */
  /* Additional commands for high-resolution and one-time measurement modes: */
  k_bh1750_cont_high_res_mode_cmd        = 0x10, /**< Continuously measure in high-resolution mode. */
  k_bh1750_cont_high_res_mode2_cmd       = 0x11, /**< Continuously measure in high-resolution mode 2. */
  k_bh1750_one_time_high_res_mode_cmd    = 0x20, /**< Single high-resolution measurement mode. */
  k_bh1750_one_time_high_res_mode2_cmd   = 0x21, /**< Single high-resolution measurement mode 2. */
  k_bh1750_one_time_low_res_mode_cmd     = 0x23, /**< Single low-resolution measurement mode. */
  k_bh1750_change_meas_time_high_bit_cmd = 0x40, /**< Adjust measurement time (high bit). */
  k_bh1750_change_meas_time_low_bit_cmd  = 0x60, /**< Adjust measurement time (low bit). */
} bh1750_commands_t;

/**
 * @brief Operational states of the BH1750 sensor.
 *
 * Represents the various states the BH1750 sensor can be in, including normal operation,
 * initialization, and error conditions.
 */
typedef enum : uint8_t {
  k_bh1750_ready              = 0x00, /**< Sensor is initialized and ready to read data. */
  k_bh1750_data_updated       = 0x01, /**< Sensor data has been updated and is available. */
  k_bh1750_uninitialized      = 0x10, /**< Sensor has not been initialized. */
  k_bh1750_error              = 0xF0, /**< General catch-all error state. */
  k_bh1750_power_on_error     = 0xA1, /**< Error occurred during power-on. */
  k_bh1750_reset_error        = 0xA2, /**< Error occurred during a reset operation. */
  k_bh1750_cont_low_res_error = 0xA3, /**< Error occurred setting continuous low-resolution mode. */
  k_bh1750_power_cycle_error  = 0xA4, /**< Error occurred during a power cycle operation. */
} bh1750_states_t;

/* Structs ********************************************************************/

/**
 * @brief Data structure for managing BH1750 sensor information.
 *
 * Contains essential data for interfacing with the BH1750 sensor, including I2C
 * communication details, light intensity readings, and retry management for
 * error handling and reinitialization.
 */
typedef struct {
  uint8_t    i2c_address;        /**< I2C address for communication with the sensor. */
  uint8_t    i2c_bus;            /**< I2C bus number the sensor is connected to. */
  float      lux;                /**< Latest light intensity reading from the sensor, in lux. */
  uint8_t    state;              /**< Current state of the sensor (see bh1750_states_t). */
  uint8_t    retry_count;        /**< Counter for consecutive reinitialization attempts. */
  uint32_t   retry_interval;     /**< Current interval between retry attempts, increases exponentially. */
  TickType_t last_attempt_ticks; /**< Tick count of the last reinitialization attempt. */
} bh1750_data_t;

/* Public Functions ***********************************************************/

/**
 * @brief Converts BH1750 sensor data to a JSON string.
 *
 * Converts the light intensity data in a `bh1750_data_t` structure to a 
 * dynamically allocated JSON string. The caller must free the memory.
 *
 * @param[in] data Pointer to the `bh1750_data_t` structure with valid sensor data.
 *
 * @return 
 * - Pointer to the JSON-formatted string on success.
 * - `NULL` if memory allocation fails.
 */
char *bh1750_data_to_json(const bh1750_data_t *data);

/**
 * @brief Initializes the BH1750 sensor in continuous high-resolution mode.
 *
 * Configures the BH1750 sensor for light intensity measurement. Sets up I2C, 
 * powers on the sensor, resets it, and applies default settings.
 *
 * @param[in,out] sensor_data Pointer to the `bh1750_data_t` structure holding
 *                            initialization parameters and state.
 *
 * @return 
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` codes on failure.
 *
 * @note 
 * - Call this function during system initialization.
 */
esp_err_t bh1750_init(void *sensor_data);

/**
 * @brief Reads light intensity data from the BH1750 sensor.
 *
 * Retrieves the latest measurement from the BH1750 sensor and updates the `lux`
 * field in the provided `bh1750_data_t` structure.
 *
 * @param[in,out] sensor_data Pointer to the `bh1750_data_t` structure to store
 *                            the sensor data and read status.
 *
 * @return 
 * - `ESP_OK`   on successful read.
 * - `ESP_FAIL` on failure.
 *
 * @note 
 * - Ensure the sensor is initialized with `bh1750_init` before calling.
 */
esp_err_t bh1750_read(bh1750_data_t *sensor_data);

/**
 * @brief Handles reinitialization and recovery for the BH1750 sensor.
 *
 * Implements exponential backoff for reinitialization attempts when errors are
 * detected. Resets retry counters on successful recovery.
 *
 * @param[in,out] sensor_data Pointer to the `bh1750_data_t` structure managing
 *                            the sensor state and retry information.
 *
 * @note 
 * - Call this function periodically within `bh1750_tasks`.
 * - Limits retries based on `bh1750_max_backoff_interval`.
 */
void bh1750_reset_on_error(bh1750_data_t *sensor_data);

/**
 * @brief Executes periodic tasks for the BH1750 sensor.
 *
 * Periodically reads data and handles errors for the BH1750 sensor. Uses
 * `bh1750_reset_on_error` for recovery. Intended to run in a FreeRTOS task.
 *
 * @param[in,out] sensor_data Pointer to the `bh1750_data_t` structure for managing
 *                            sensor data and error recovery.
 *
 * @note 
 * - Should run at intervals defined by `bh1750_polling_rate_ticks`.
 * - Handles error recovery internally to maintain stable operation.
 */
void bh1750_tasks(void *sensor_data);

#ifdef __cplusplus
}
#endif

#endif /* SAFEHAT_WORKNET_BH1750_HAL_H */

