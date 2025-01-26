/* components/camera/ov7670_hal/include/ov7670_hal.h */

#ifndef SAFEHAT_WORKNET_OV7670_HAL_H
#define SAFEHAT_WORKNET_OV7670_HAL_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ******************************************************************/

extern const uint8_t    ov7670_scl_io;             /**< GPIO pin for I2C Clock (SCL) */
extern const uint8_t    ov7670_sda_io;             /**< GPIO pin for I2C Data (SDA) */
extern const uint32_t   ov7670_polling_rate_ticks; /**< Polling rate for checking camera configuration status in ticks */
extern const uint8_t    ov7670_i2c_address;        /**< OV7670 I2C address */
extern const i2c_port_t ov7670_i2c_bus;            /**< OV7670 I2C bus */
extern const uint32_t   ov7670_i2c_freq_hz;        /**< OV7670 I2C Freq in Hz (100k) */

/* Enums **********************************************************************/

/**
 * @brief Represents the operational states of the OV7670 camera module.
 *
 * Defines various states to track the operational status of the camera.
 */
typedef enum : uint8_t {
  k_ov7670_uninitialized = 0x00, /**< Camera is not initialized. */
  k_ov7670_ready         = 0x01, /**< Camera is initialized and ready for use. */
  k_ov7670_config_error  = 0xF0, /**< A configuration error occurred. */
} ov7670_states_t;

/**
 * @brief Supported resolutions for the OV7670 camera.
 *
 * Defines the image resolutions the camera can produce.
 */
typedef enum : uint8_t {
  k_ov7670_res_vga  = 0x00, /**< VGA resolution (640x480), default setting. */
  k_ov7670_res_qvga = 0x10, /**< QVGA resolution (320x240). */
} ov7670_resolution_t;

/**
 * @brief Supported output formats for the OV7670 camera.
 *
 * Specifies the data formats the camera can output.
 */
typedef enum : uint8_t {
  k_ov7670_output_yuv   = 0x00, /**< YUV422 format, default setting. */
  k_ov7670_output_rgb   = 0x04, /**< RGB565 format. */
  k_ov7670_output_bayer = 0x01, /**< Bayer RAW format. */
} ov7670_output_format_t;

/**
 * @brief Supported clock dividers for the OV7670 camera.
 *
 * Defines available clock division options for the camera module.
 */
typedef enum : uint8_t {
  k_ov7670_clk_div_1 = 0x00, /**< No clock division (divider = 1). */
  k_ov7670_clk_div_2 = 0x01, /**< Clock divided by 2. */
  k_ov7670_clk_div_4 = 0x02, /**< Clock divided by 4. */
  k_ov7670_clk_div_8 = 0x03, /**< Clock divided by 8. */
} ov7670_clock_divider_t;

/**
 * @brief Register addresses for the OV7670 camera.
 *
 * Lists the internal register addresses for configuring the camera module.
 */
typedef enum : uint8_t {
  k_ov7670_reg_com7   = 0x12, /**< Common Control 7 register. */
  k_ov7670_reg_clkrc  = 0x11, /**< Clock Control register. */
  k_ov7670_reg_rgb444 = 0x8C, /**< RGB444 Control register. */
  k_ov7670_reg_com15  = 0x40, /**< Common Control 15 register. */
  k_ov7670_reg_tslb   = 0x3A, /**< Line Buffer Control register. */
  k_ov7670_reg_com3   = 0x0C, /**< Common Control 3 register. */
  k_ov7670_reg_com14  = 0x3E, /**< Common Control 14 register. */
} ov7670_register_t;

/* Structs ********************************************************************/

/**
 * @brief Configuration settings for the OV7670 camera module.
 *
 * Contains the resolution, output format, and clock divider settings
 * required to configure the OV7670 camera module.
 */
typedef struct {
  ov7670_resolution_t    resolution;    /**< Desired resolution (e.g., QVGA, VGA). */
  ov7670_output_format_t output_format; /**< Desired output format (e.g., RGB, YUV). */
  ov7670_clock_divider_t clock_divider; /**< Clock divider for the internal pixel clock. */
} ov7670_config_t;

/**
 * @brief Data structure for managing OV7670 camera module state and configuration.
 *
 * Tracks the current state of the camera, retries for configuration errors,
 * timing information for retry intervals, and the active configuration.
 */
typedef struct {
  uint8_t         state;              /**< Current state of the camera module (see ov7670_states_t). */
  uint8_t         retries;            /**< Retry count for configuration attempts after errors. */
  uint32_t        retry_interval;     /**< Interval between retries in ticks. */
  uint32_t        last_attempt_ticks; /**< Tick count at the last configuration attempt. */
  ov7670_config_t config;             /**< Current active configuration settings. */
} ov7670_data_t;

/* Public Functions ***********************************************************/

/**
 * @brief Initializes the OV7670 camera module.
 *
 * Configures the I2C interface and initializes the OV7670 camera module with
 * default settings, preparing it for operation. Verifies the camera's readiness
 * and logs errors if initialization fails.
 *
 * @param[in,out] sensor_data Pointer to the `ov7670_data_t` structure holding
 *                            the camera's operational state.
 *
 * @return 
 * - `ESP_OK` on successful initialization.
 * - Error codes from `esp_err_t` on failure (e.g., communication errors).
 */
esp_err_t ov7670_init(ov7670_data_t *sensor_data);

/**
 * @brief Applies a new set of configuration parameters to the OV7670 module.
 *
 * Updates the OV7670 camera's settings using the `config` member of the provided
 * `ov7670_data_t` structure. Allows real-time reconfiguration without a reset.
 *
 * @param[in,out] camera_data Pointer to the `ov7670_data_t` structure containing
 *                            the new configuration.
 *
 * @return 
 * - `ESP_OK`   if configuration is applied successfully.
 * - `ESP_FAIL` if configuration fails (e.g., invalid parameters or I2C errors).
 */
esp_err_t ov7670_configure(ov7670_data_t *camera_data);

/**
 * @brief Handles error recovery for the OV7670 module using retries.
 *
 * Attempts to recover the OV7670 module from errors by reconfiguring it. Employs
 * an exponential backoff strategy for retries, adjusting intervals between attempts.
 *
 * @param[in,out] sensor_data Pointer to the `ov7670_data_t` structure managing
 *                            the camera's state and retry information.
 *
 * @note 
 * - Call this function when an error is detected in the camera's operation.
 * - The retry mechanism reduces system load during repeated failures.
 */
void ov7670_reset_on_error(ov7670_data_t *sensor_data);

/**
 * @brief Executes periodic tasks for the OV7670 camera module.
 *
 * Monitors the camera's status and handles reconfiguration or error recovery
 * as needed. This function should run in a periodic task or timer loop.
 *
 * @param[in,out] sensor_data Pointer to the `ov7670_data_t` structure managing
 *                            the camera's state and configuration.
 *
 * @note 
 * - Ensure this function is invoked at regular intervals for continuous operation.
 * - Includes all necessary logic for error recovery and maintenance.
 */
void ov7670_tasks(void *sensor_data);

#ifdef __cplusplus
}
#endif

#endif /* SAFEHAT_WORKNET_OV7670_HAL_H */
