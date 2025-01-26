/* components/sensors/mpu6050_hal/include/mpu6050_hal.h */

#ifndef SAFEHAT_WORKNET_MPU6050_HAL_H
#define SAFEHAT_WORKNET_MPU6050_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "error_handler.h"

/* Constants ******************************************************************/

extern const uint8_t    mpu6050_i2c_address;        /**< I2C address for the MPU6050 sensor (default 0x68, configurable to 0x69). */
extern const i2c_port_t mpu6050_i2c_bus;            /**< I2C bus number used by the ESP32 for MPU6050 communication. */
extern const char      *mpu6050_tag;                /**< Tag for ESP_LOG messages related to the MPU6050 sensor. */
extern const uint8_t    mpu6050_scl_io;             /**< GPIO pin for I2C Serial Clock Line (SCL) for MPU6050. */
extern const uint8_t    mpu6050_sda_io;             /**< GPIO pin for I2C Serial Data Line (SDA) for MPU6050. */
extern const uint32_t   mpu6050_i2c_freq_hz;        /**< I2C bus frequency for MPU6050 communication (default 100 kHz). */
extern const uint32_t   mpu6050_polling_rate_ticks; /**< Polling interval for MPU6050 sensor reads in system ticks. */
extern const uint8_t    mpu6050_sample_rate_div;    /**< Sample rate divider for MPU6050 (default divides gyro rate). */
extern const uint8_t    mpu6050_config_dlpf;        /**< Digital Low Pass Filter (DLPF) setting for noise reduction. */
extern const uint8_t    mpu6050_int_io;             /**< GPIO pin for MPU6050 interrupt signal (INT pin). */
extern const uint8_t    mpu6050_max_retries;        /**< Maximum retry attempts for MPU6050 reinitialization. */
extern const uint32_t   mpu6050_initial_retry_interval; /**< Initial retry interval for MPU6050 in system ticks. */
extern const uint32_t   mpu6050_max_backoff_interval;   /**< Maximum backoff interval for MPU6050 retries in ticks. */
extern const uint8_t    mpu6050_allowed_fail_attempts;  /**< Number of allowed consecutive failures before reset. */

/* Enums **********************************************************************/

/**
 * @brief Enumeration of interrupt modes for the MPU6050 sensor.
 *
 * Defines the possible interrupt sources for the MPU6050 sensor. The INT pin on the MPU6050
 * asserts when any of the enabled interrupts occur. To identify which interrupt(s) triggered,
 * the microcontroller must read the INT_STATUS register.
 */
typedef enum : uint8_t {
  k_mpu6050_int_mode_data_ready,       /**< Interrupt triggered when new data is available. */
  k_mpu6050_int_mode_motion_detection, /**< Interrupt triggered by motion detection exceeding a threshold. */
  k_mpu6050_int_mode_zero_motion,      /**< Interrupt triggered by no motion detected for a specified duration. */
  k_mpu6050_int_mode_free_fall,        /**< Interrupt triggered by free-fall conditions. */
  k_mpu6050_int_mode_fifo_overflow,    /**< Interrupt triggered by a FIFO buffer overflow. */
  k_mpu6050_int_mode_i2c_mst,          /**< Interrupt triggered by the I2C Master module. */
  k_mpu6050_int_mode_dmp,              /**< Interrupt triggered by the Digital Motion Processor (DMP). */
  k_mpu6050_int_mode_pll_ready,        /**< Interrupt triggered when the Phase-Locked Loop (PLL) is ready. */
} mpu6050_interrupt_mode_t;

/**
 * @brief Enumeration of interrupt enable bits for the MPU6050 sensor.
 *
 * Represents the bits used to enable specific interrupts in the MPU6050's INT_ENABLE register.
 * Each bit corresponds to one of the available interrupt sources.
 */
typedef enum : uint8_t {
  k_mpu6050_int_enable_data_rdy   = 0x01, /**< Enable bit for Data Ready interrupt. */
  k_mpu6050_int_enable_dmp_int    = 0x02, /**< Enable bit for DMP interrupt. */
  k_mpu6050_int_enable_pll_rdy    = 0x04, /**< Enable bit for PLL Ready interrupt. */
  k_mpu6050_int_enable_i2c_mst    = 0x08, /**< Enable bit for I2C Master interrupt. */
  k_mpu6050_int_enable_fifo_oflow = 0x10, /**< Enable bit for FIFO Overflow interrupt. */
  k_mpu6050_int_enable_zmot       = 0x20, /**< Enable bit for Zero Motion Detection interrupt. */
  k_mpu6050_int_enable_mot        = 0x40, /**< Enable bit for Motion Detection interrupt. */
  k_mpu6050_int_enable_ff         = 0x80, /**< Enable bit for Free-Fall Detection interrupt. */
} mpu6050_int_enable_bits_t;

/**
 * @brief Enumeration of event bits for MPU6050 interrupt events.
 *
 * Represents the bits used in an event group to signal specific interrupt events
 * from the MPU6050 sensor. These event bits correspond to the interrupt sources
 * defined by the sensor.
 */
typedef enum : uint8_t {
  k_mpu6050_event_data_ready      = 0x01, /**< Bit for Data Ready event. */
  k_mpu6050_event_motion_detected = 0x02, /**< Bit for Motion Detected event. */
  k_mpu6050_event_zero_motion     = 0x04, /**< Bit for Zero Motion event. */
  k_mpu6050_event_free_fall       = 0x08, /**< Bit for Free-Fall event. */
  k_mpu6050_event_fifo_overflow   = 0x10, /**< Bit for FIFO Overflow event. */
  k_mpu6050_event_i2c_mst         = 0x20, /**< Bit for I2C Master event. */
  k_mpu6050_event_dmp             = 0x40, /**< Bit for Digital Motion Processor (DMP) event. */
  k_mpu6050_event_pll_ready       = 0x80, /**< Bit for Phase-Locked Loop (PLL) Ready event. */
} mpu6050_event_bits_t;

/**
 * @brief Enumeration of MPU6050 sensor states.
 *
 * Defines the possible states of the MPU6050 sensor for tracking its operational
 * status, error handling, and data acquisition tasks.
 */
typedef enum : uint8_t {
  k_mpu6050_ready            = 0x00, /**< Sensor is initialized and ready to read data. */
  k_mpu6050_data_updated     = 0x01, /**< New sensor data has been updated. */
  k_mpu6050_uninitialized    = 0x10, /**< Sensor is not initialized. */
  k_mpu6050_error            = 0xF0, /**< General error state. */
  k_mpu6050_power_on_error   = 0xA1, /**< Error occurred during power-on initialization. */
  k_mpu6050_reset_error      = 0xA3, /**< Error occurred while resetting the sensor. */
  k_mpu6050_dlp_config_error = 0xA4, /**< Error occurred while configuring the Digital Low Pass Filter (DLPF). */
} mpu6050_states_t;

/**
 * @brief Enumeration of I2C commands for the MPU6050 sensor.
 *
 * Defines commands for configuring and operating the MPU6050 sensor, including power management,
 * data reading, interrupt configuration, and sensitivity settings for the accelerometer and gyroscope.
 * Also includes configuration values for features like Digital Low Pass Filter (DLPF) and full-scale ranges.
 */
typedef enum : uint8_t {
  /* Power Management Commands */
  k_mpu6050_power_down_cmd      = 0x40, /**< Command to power down the sensor (enter sleep mode). */
  k_mpu6050_power_on_cmd        = 0x00, /**< Command to power on the sensor (wake up from sleep). */
  k_mpu6050_reset_cmd           = 0x80, /**< Command to reset the sensor. */
  k_mpu6050_pwr_mgmt_1_cmd      = 0x6B, /**< Power Management 1 register. */
  
  /* Configuration Commands */
  k_mpu6050_smplrt_div_cmd      = 0x19, /**< Sample Rate Divider register. */
  k_mpu6050_config_cmd          = 0x1A, /**< Configuration register for DLPF and FSYNC. */
  k_mpu6050_gyro_config_cmd     = 0x1B, /**< Gyroscope Configuration register (full-scale range). */
  k_mpu6050_accel_config_cmd    = 0x1C, /**< Accelerometer Configuration register (full-scale range). */
  k_mpu6050_who_am_i_cmd        = 0x75, /**< WHO_AM_I register to verify sensor identity. */
  
  /* Interrupt Configuration Commands */
  k_mpu6050_int_enable_cmd      = 0x38, /**< Interrupt Enable register. */
  k_mpu6050_int_status_cmd      = 0x3A, /**< Interrupt Status register. */

  /* Configuration Values */
  k_mpu6050_who_am_i_response   = 0x68, /**< Expected response from the WHO_AM_I register. */
  k_mpu6050_config_dlpf_260hz   = 0x00, /**< DLPF: 260Hz bandwidth, 0ms delay. */
  k_mpu6050_config_dlpf_184hz   = 0x01, /**< DLPF: 184Hz bandwidth, 2.0ms delay. */
  k_mpu6050_config_dlpf_94hz    = 0x02, /**< DLPF: 94Hz bandwidth, 3.0ms delay. */
  k_mpu6050_config_dlpf_44hz    = 0x03, /**< DLPF: 44Hz bandwidth, 4.9ms delay. */
  k_mpu6050_config_dlpf_21hz    = 0x04, /**< DLPF: 21Hz bandwidth, 8.5ms delay. */
  k_mpu6050_config_dlpf_10hz    = 0x05, /**< DLPF: 10Hz bandwidth, 13.8ms delay. */
  k_mpu6050_config_dlpf_5hz     = 0x06, /**< DLPF: 5Hz bandwidth, 19.0ms delay. */
  k_mpu6050_gyro_fs_250dps      = 0x00, /**< Gyroscope full-scale range: ±250°/s. */
  k_mpu6050_gyro_fs_500dps      = 0x08, /**< Gyroscope full-scale range: ±500°/s. */
  k_mpu6050_gyro_fs_1000dps     = 0x10, /**< Gyroscope full-scale range: ±1000°/s. */
  k_mpu6050_gyro_fs_2000dps     = 0x18, /**< Gyroscope full-scale range: ±2000°/s. */
  k_mpu6050_accel_fs_2g         = 0x00, /**< Accelerometer full-scale range: ±2g. */
  k_mpu6050_accel_fs_4g         = 0x08, /**< Accelerometer full-scale range: ±4g. */
  k_mpu6050_accel_fs_8g         = 0x10, /**< Accelerometer full-scale range: ±8g. */
  k_mpu6050_accel_fs_16g        = 0x18, /**< Accelerometer full-scale range: ±16g. */

  /* Data Register Commands */
  k_mpu6050_accel_xout_h_cmd    = 0x3B, /**< Accelerometer X-axis High byte. */
  k_mpu6050_accel_xout_l_cmd    = 0x3C, /**< Accelerometer X-axis Low byte. */
  k_mpu6050_accel_yout_h_cmd    = 0x3D, /**< Accelerometer Y-axis High byte. */
  k_mpu6050_accel_yout_l_cmd    = 0x3E, /**< Accelerometer Y-axis Low byte. */
  k_mpu6050_accel_zout_h_cmd    = 0x3F, /**< Accelerometer Z-axis High byte. */
  k_mpu6050_accel_zout_l_cmd    = 0x40, /**< Accelerometer Z-axis Low byte. */
  k_mpu6050_gyro_xout_h_cmd     = 0x43, /**< Gyroscope X-axis High byte. */
  k_mpu6050_gyro_xout_l_cmd     = 0x44, /**< Gyroscope X-axis Low byte. */
  k_mpu6050_gyro_yout_h_cmd     = 0x45, /**< Gyroscope Y-axis High byte. */
  k_mpu6050_gyro_yout_l_cmd     = 0x46, /**< Gyroscope Y-axis Low byte. */
  k_mpu6050_gyro_zout_h_cmd     = 0x47, /**< Gyroscope Z-axis High byte. */
  k_mpu6050_gyro_zout_l_cmd     = 0x48, /**< Gyroscope Z-axis Low byte. */

  /* Unused or Optional Commands */
  k_mpu6050_pwr_mgmt_2_cmd      = 0x6C, /**< Power Management 2 register. */
  k_mpu6050_fifo_en_cmd         = 0x23, /**< FIFO Enable register. */
  k_mpu6050_fifo_count_h_cmd    = 0x72, /**< FIFO Count High byte. */
  k_mpu6050_fifo_count_l_cmd    = 0x73, /**< FIFO Count Low byte. */
  k_mpu6050_fifo_r_w_cmd        = 0x74, /**< FIFO Read/Write register. */
  k_mpu6050_temp_out_h_cmd      = 0x41, /**< Temperature High byte. */
  k_mpu6050_temp_out_l_cmd      = 0x42, /**< Temperature Low byte. */

  /* Factory-Level Testing Commands */
  k_mpu6050_self_test_x_cmd     = 0x0D, /**< Self-test register for X-axis (factory testing). */
  k_mpu6050_self_test_y_cmd     = 0x0E, /**< Self-test register for Y-axis (factory testing). */
  k_mpu6050_self_test_z_cmd     = 0x0F, /**< Self-test register for Z-axis (factory testing). */
  k_mpu6050_self_test_a_cmd     = 0x10, /**< Self-test register for Accelerometer (factory testing). */
} mpu6050_commands_t;

/* Structs ********************************************************************/

/**
 * @brief Structure to hold the accelerometer configuration for the MPU6050.
 *
 * Contains the register value for setting the full-scale range and the corresponding
 * scaling factor for converting raw acceleration data to gravitational units (g).
 *
 * The scaling factor is calculated as:
 *   Scaling Factor = (Full-Scale Range) / (Maximum Raw Value)
 * where the Maximum Raw Value is 32768 for the MPU6050.
 */
typedef struct {
  uint8_t accel_config; /**< Register value for setting the accelerometer full-scale range. */
  float   accel_scale;  /**< Scaling factor to convert raw data to acceleration in g. */
} mpu6050_accel_config_t;

/**
 * @brief Structure to hold the gyroscope configuration for the MPU6050.
 *
 * Contains the register value for setting the full-scale range and the corresponding
 * scaling factor for converting raw gyroscope data to angular velocity in degrees per second (°/s).
 *
 * The scaling factor is calculated as:
 *   Scaling Factor = (Full-Scale Range) / (Maximum Raw Value)
 * where the Maximum Raw Value is 32768 for the MPU6050.
 */
typedef struct {
  uint8_t gyro_config; /**< Register value for setting the gyroscope full-scale range. */
  float   gyro_scale;  /**< Scaling factor to convert raw data to angular velocity in °/s. */
} mpu6050_gyro_config_t;

/**
 * @brief Structure to store MPU6050 sensor data and state.
 *
 * Contains accelerometer and gyroscope readings, temperature, operational state,
 * semaphore for signaling data readiness, and error handling through the error_handler_t structure.
 */
typedef struct {
  uint8_t           i2c_address;    /**< I2C address used for communication with the sensor. */
  uint8_t           i2c_bus;        /**< I2C bus number used for communication. */
  float             accel_x;        /**< Measured X-axis acceleration in g. */
  float             accel_y;        /**< Measured Y-axis acceleration in g. */
  float             accel_z;        /**< Measured Z-axis acceleration in g. */
  float             gyro_x;         /**< Measured X-axis angular velocity in °/s. */
  float             gyro_y;         /**< Measured Y-axis angular velocity in °/s. */
  float             gyro_z;         /**< Measured Z-axis angular velocity in °/s. */
  float             temperature;    /**< Measured temperature from the sensor in degrees Celsius. */
  uint8_t           state;          /**< Current operational state of the sensor (see `mpu6050_states_t`). */
  SemaphoreHandle_t data_ready_sem; /**< Semaphore to signal when new data is available. */
  error_handler_t   error_handler;  /**< Error handler for managing sensor errors and recovery. */
} mpu6050_data_t;

/* Public Functions ***********************************************************/

/**
 * @brief Converts MPU6050 sensor data to a JSON string.
 *
 * Converts the accelerometer and gyroscope data in a `mpu6050_data_t` 
 * structure to a dynamically allocated JSON string. The caller must free the memory.
 *
 * @param[in] data Pointer to the `mpu6050_data_t` structure with valid sensor data.
 *
 * @return 
 * - Pointer to the JSON-formatted string on success.
 * - `NULL` if memory allocation fails.
 * 
 * @note The returned string should be freed by the caller to prevent memory leaks.
 */
char *mpu6050_data_to_json(const mpu6050_data_t *data);

/**
 * @brief Initializes the MPU6050 sensor in continuous measurement mode.
 *
 * Configures the MPU6050 sensor for accelerometer and gyroscope data collection.
 * Sets up I2C, applies default settings, and initializes the sensor for continuous
 * measurement mode.
 *
 * @param[in,out] sensor_data Pointer to the `mpu6050_data_t` structure holding
 *                            initialization parameters and state.
 *
 * @return 
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` code on failure.
 *
 * @note 
 * - Call this function during system initialization.
 */
esp_err_t mpu6050_init(void *sensor_data);

/**
 * @brief Reads accelerometer and gyroscope data from the MPU6050 sensor.
 *
 * Retrieves the latest measurements from the MPU6050 sensor and updates the 
 * `mpu6050_data_t` structure with the new data.
 *
 * @param[in,out] sensor_data Pointer to the `mpu6050_data_t` structure to store
 *                            the sensor data and read status.
 *
 * @return 
 * - `ESP_OK`   on successful read.
 * - `ESP_FAIL` on failure.
 *
 * @note 
 * - Ensure the sensor is initialized with `mpu6050_init` before calling.
 */
esp_err_t mpu6050_read(mpu6050_data_t *sensor_data);

/**
 * @brief Executes periodic tasks for the MPU6050 sensor.
 *
 * Periodically reads data and handles errors for the MPU6050 sensor using
 * the error handler for recovery. Intended to run in a FreeRTOS task.
 *
 * @param[in,out] sensor_data Pointer to the `mpu6050_data_t` structure for managing
 *                            sensor data and error recovery.
 *
 * @note 
 * - Should run at intervals defined by `mpu6050_polling_rate_ticks`.
 * - Uses error_handler_t for error recovery to maintain stable operation.
 */
void mpu6050_tasks(void *sensor_data);

#ifdef __cplusplus
}
#endif

#endif /* SAFEHAT_WORKNET_MPU6050_HAL_H */

