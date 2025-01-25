/* components/sensors/ccs811_hal/include/ccs811_hal.h */

#ifndef SAFEHAT_WORKNET_CCS811_HAL_H
#define SAFEHAT_WORKNET_CCS811_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

/* Constants ******************************************************************/

extern const uint8_t    ccs811_i2c_address; /**< Default I2C address for the CCS811 sensor (default 0x5A). */
extern const i2c_port_t ccs811_i2c_bus;     /**< I2C bus number used for communication. */
extern const char      *ccs811_tag;         /**< Tag used for logging messages related to the CCS811 sensor. */
extern const uint8_t    ccs811_scl_io;      /**< GPIO pin for I2C clock line (SCL). */
extern const uint8_t    ccs811_sda_io;      /**< GPIO pin for I2C data line (SDA). */
extern const uint8_t    ccs811_wake_io;     /**< GPIO pin for the sensor's wake-up function. */
extern const uint8_t    ccs811_rst_io;      /**< GPIO pin for the sensor's reset function. */
extern const uint8_t    ccs811_int_io;      /**< GPIO pin for the sensor's interrupt function (optional). */

/* Enums **********************************************************************/

/**
 * @brief GPIO pin states for CCS811 control signals
 */
typedef enum : uint8_t {
  k_ccs811_gpio_low  = 0, /**< GPIO pin low state */
  k_ccs811_gpio_high = 1  /**< GPIO pin high state */
} ccs811_gpio_state_t;

/**
 * @brief CCS811 register addresses and commands
 */
typedef enum : uint8_t {
  k_ccs811_cmd_app_start = 0xF4,      /**< Start application command */
  k_ccs811_reg_alg_result_data = 0x02 /**< Algorithm result data register */
} ccs811_registers_t;

/**
 * @brief CCS811 data buffer sizes
 */
typedef enum : uint8_t {
  k_ccs811_alg_data_len = 4 /**< Length of algorithm result data */
} ccs811_buffer_sizes_t;

/**
 * @brief Enumeration of CCS811 sensor states.
 *
 * Defines the possible operational states of the CCS811 air quality sensor, 
 * including normal operation, data updates, initialization status, and error conditions.
 */
typedef enum : uint8_t {
  k_ccs811_ready           = 0x00, /**< Sensor is initialized and ready to provide data. */
  k_ccs811_data_updated    = 0x01, /**< New data is available from the sensor. */
  k_ccs811_uninitialized   = 0x10, /**< Sensor has not been initialized. */
  k_ccs811_error           = 0xF0, /**< General catch-all error state. */
  k_ccs811_power_on_error  = 0xA1, /**< Error occurred during power-on. */
  k_ccs811_reset_error     = 0xA2, /**< Error occurred during reset. */
  k_ccs811_app_start_error = 0xA3, /**< Error occurred when starting the sensor's application. */
  k_ccs811_read_error      = 0xA4, /**< Error occurred while reading sensor data. */
} ccs811_states_t;

/* Structs ********************************************************************/

/**
 * @brief Structure for managing CCS811 sensor data and status.
 *
 * Contains I2C communication details, the latest sensor measurements, and variables 
 * for handling error recovery and reinitialization.
 */
typedef struct {
  uint8_t    i2c_address;        /**< I2C address used for communication with the sensor. */
  uint8_t    i2c_bus;            /**< I2C bus number the sensor is connected to. */
  uint16_t   eco2;               /**< Latest equivalent CO2 (eCO2) reading in parts per million (ppm). */
  uint16_t   tvoc;               /**< Latest Total Volatile Organic Compounds (TVOC) reading in parts per billion (ppb). */
  uint8_t    state;              /**< Current operational state of the sensor (see ccs811_states_t). */
  uint8_t    retry_count;        /**< Number of consecutive reinitialization attempts. */
  uint32_t   retry_interval;     /**< Current interval between reinitialization attempts, increasing with retries. */
  TickType_t last_attempt_ticks; /**< Tick count of the last reinitialization attempt. */
} ccs811_data_t;

/* Public Functions ***********************************************************/

/**
 * @brief Converts CCS811 sensor data to a JSON string.
 *
 * Converts the air quality data in a `ccs811_data_t` structure to a 
 * dynamically allocated JSON-formatted string. The caller must free the memory.
 *
 * @param[in] data Pointer to the `ccs811_data_t` structure containing sensor 
 *                 readings.
 * 
 * @return 
 * - Pointer to the JSON-formatted string on success.
 * - `NULL` if memory allocation fails.
 */
char *ccs811_data_to_json(const ccs811_data_t *data);

/**
 * @brief Initializes the CCS811 sensor.
 *
 * Configures the I2C interface and prepares the CCS811 sensor for continuous 
 * air quality monitoring.
 *
 * @param[in,out] sensor_data Pointer to the `ccs811_data_t` structure for storing
 *                            sensor-specific data and state.
 * 
 * @return 
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` codes on failure.
 */
esp_err_t ccs811_init(void *sensor_data);

/**
 * @brief Reads eCO2 and TVOC data from the CCS811 sensor.
 *
 * Updates the `ccs811_data_t` structure with the latest air quality data from the sensor.
 *
 * @param[in,out] sensor_data Pointer to the `ccs811_data_t` structure for storing 
 *                            sensor readings.
 * 
 * @return 
 * - `ESP_OK`   on success.
 * - `ESP_FAIL` on failure.
 */
esp_err_t ccs811_read(ccs811_data_t *sensor_data);

/**
 * @brief Handles reinitialization and error recovery for the CCS811 sensor.
 *
 * Implements exponential backoff and retries when the sensor encounters errors.
 *
 * @param[in,out] sensor_data Pointer to the `ccs811_data_t` structure managing 
 *                            the sensor state and retry logic.
 */
void ccs811_reset_on_error(ccs811_data_t *sensor_data);

/**
 * @brief Executes periodic tasks for the CCS811 sensor.
 *
 * Periodically reads air quality data from the sensor, handles error recovery, 
 * and transmits data to a web server.
 *
 * @param[in,out] sensor_data Pointer to the `ccs811_data_t` structure managing 
 *                            sensor data and state.
 */
void ccs811_tasks(void *sensor_data);

#ifdef __cplusplus
}
#endif

#endif /* SAFEHAT_WORKNET_CCS811_HAL_H */

