/* components/controllers/pca9685_hal/include/pca9685_hal.h */

#ifndef MESHNET_PCA9685_HAL_H
#define MESHNET_PCA9685_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "motor_common.h"

/* Macros *********************************************************************/

#define PCA9685_MOTORS_PER_BOARD (16) /**< Number of motors per PCA9685 board */

/* Constants ******************************************************************/

extern const uint8_t    pca9685_scl_io;           /**< GPIO pin for I2C Serial Clock Line */
extern const uint8_t    pca9685_sda_io;           /**< GPIO pin for I2C Serial Data Line */
extern const uint32_t   pca9685_i2c_freq_hz;      /**< I2C Bus Frequency in Hz */
extern const uint8_t    pca9685_i2c_address;      /**< Base I2C address for PCA9685 */
extern const i2c_port_t pca9685_i2c_bus;          /**< I2C bus for PCA9685 */
extern const uint32_t   pca9685_osc_freq;         /**< Internal Oscillator Frequency (25 MHz) */
extern const uint16_t   pca9685_pwm_resolution;   /**< 12-bit PWM resolution (4096 steps) */
extern const uint16_t   pca9685_default_pwm_freq; /**< Default PWM frequency (50 Hz for servos) */
extern const uint16_t   pca9685_max_pwm_value;    /**< Maximum value for PWM duty cycle (4095) */
extern const uint16_t   pca9685_pwm_period_us;    /**< Total PWM period for 50Hz (20000 µs) */
extern const char      *pca9685_tag;              /**< Tag for logs */
extern const uint8_t    pca9685_step_size_deg;    /**< Step size in degrees for gradual movement */
extern const uint32_t   pca9685_step_delay_ms;    /**< Delay in milliseconds between steps */
extern const float      pca9685_default_angle;    /**< Default angle for motors */

/* Enums **********************************************************************/

/**
 * @brief States representing the operational status of the PCA9685.
 *
 * Defines the possible states of a PCA9685 board, including ready, uninitialized, and various error states.
 */
typedef enum : uint8_t {
  k_pca9685_ready               = 0x00, /**< The PCA9685 board is ready and operational. */
  k_pca9685_uninitialized       = 0x01, /**< The PCA9685 board is not yet initialized. */
  k_pca9685_error               = 0xF0, /**< General catch-all error. */
  k_pca9685_power_on_error      = 0xA1, /**< Error during power-on of the board. */
  k_pca9685_reset_error         = 0xA2, /**< Error during reset of the board. */
  k_pca9685_communication_error = 0xA3  /**< Error during I2C communication. */
} pca9685_states_t;

/**
 * @brief I2C commands and register addresses for the PCA9685.
 *
 * Defines the I2C register addresses and associated commands for configuring
 * and controlling the PCA9685 PWM driver.
 *
 * @note Only the register addresses for channel 0 are explicitly defined.
 *       Addresses for other channels (1 to 15) can be calculated as follows:
 *       - Channel 1 low byte = `k_pca9685_channel0_on_l_cmd + 4`
 *       - Channel 2 low byte = `k_pca9685_channel0_on_l_cmd + 8`
 *       - Channel N low byte = `k_pca9685_channel0_on_l_cmd + (4 * N)`
 */
typedef enum : uint8_t {
  k_pca9685_mode1_cmd              = 0x00, /**< MODE1 register (mode configuration). */
  k_pca9685_mode2_cmd              = 0x01, /**< MODE2 register (output configuration). */
  k_pca9685_prescale_cmd           = 0xFE, /**< Prescaler for setting the PWM frequency. */
  k_pca9685_channel0_on_l_cmd      = 0x06, /**< Channel 0 output on time (low byte). */
  k_pca9685_channel0_on_h_cmd      = 0x07, /**< Channel 0 output on time (high byte). */
  k_pca9685_channel0_off_l_cmd     = 0x08, /**< Channel 0 output off time (low byte). */
  k_pca9685_channel0_off_h_cmd     = 0x09, /**< Channel 0 output off time (high byte). */
  k_pca9685_auto_increment_cmd     = 0x20, /**< Auto-increment for MODE1 register. */
  k_pca9685_restart_cmd            = 0x80, /**< Restart command to enable PWM after frequency changes. */
  k_pca9685_sleep_cmd              = 0x10, /**< Sleep command to put the PCA9685 into low-power mode. */
  k_pca9685_allcall_cmd            = 0x01, /**< ALLCALL command to address all PCA9685 devices. */
  k_pca9685_output_change_ack_cmd  = 0x01, /**< Change output on ACK bit in MODE2 register. */
  k_pca9685_output_change_stop_cmd = 0x00, /**< Change output on STOP condition in MODE2 register. */
  k_pca9685_output_logic_mode      = 0x04, /**< OUTDRV bit in MODE2 register to set output driver mode. */
} pca9685_commands_t;

/* Structs ********************************************************************/

/**
 * @brief Represents a PCA9685 board in the system.
 *
 * Contains information about a single PCA9685 board, including its I2C address,
 * communication bus, operational state, unique ID, and motors it controls.
 * It also supports a singly linked list structure for managing multiple boards.
 */
typedef struct pca9685_board_t {
  uint8_t                 i2c_address; /**< Base I2C address of the PCA9685 board. */
  uint8_t                 i2c_bus;     /**< I2C bus number used for communication. */
  uint8_t                 state;       /**< Current state of the PCA9685 (see pca9685_states_t). */
  uint8_t                 board_id;    /**< Unique ID for this board in multi-board setups. */
  uint8_t                 num_boards;  /**< Total number of PCA9685 boards in the system. */
  motor_t                 motors[PCA9685_MOTORS_PER_BOARD];  /**< Array representing the motors controlled by this board. */
  struct pca9685_board_t *next;        /**< Pointer to the next board in the singly linked list. */
} pca9685_board_t;

/* Public Functions ***********************************************************/

/**
 * @brief Initializes the PCA9685 PWM driver over I2C for multiple boards.
 *
 * Configures and initializes the PCA9685 PWM driver for the specified number of boards.
 * Sets the PWM frequency for controlling servos or other PWM-controlled devices.
 * Logs errors and returns a failure code if any board initialization fails.
 *
 * @param[in,out] controller_data Pointer to the head of the linked list of PCA9685 boards.
 * @param[in] num_boards          Number of PCA9685 boards to initialize.
 *
 * @return 
 * - `ESP_OK`              if all boards are successfully initialized.
 * - `ESP_ERR_INVALID_ARG` if `controller_data` is NULL or `num_boards` is zero.
 * - Other `esp_err_t` codes on initialization failure.
 *
 * @note 
 * - Default PWM frequency is set to 50 Hz, suitable for servos.
 */
esp_err_t pca9685_init(pca9685_board_t **controller_data, uint8_t num_boards);

/**
 * @brief Sets the angle for one or more servo motors on a specific PCA9685 board.
 *
 * Updates servo positions using a motor bitmask and desired angle. Converts the angle 
 * (in degrees) to the corresponding PWM pulse width and applies it to the selected motors.
 *
 * @param[in] controller_data Pointer to the linked list of PCA9685 boards.
 * @param[in] motor_mask      Bitmask indicating motors to control (e.g., 0x01 for channel 0).
 * @param[in] board_id        ID of the PCA9685 board to control.
 * @param[in] angle           Servo angle in degrees (0-180).
 *
 * @return 
 * - `ESP_OK`              if angles are successfully applied.
 * - `ESP_ERR_INVALID_ARG` if parameters are invalid (e.g., NULL `controller_data`, 
 *                         out-of-range angle, or invalid motor mask).
 * - `ESP_FAIL`            if the board ID is not found or not initialized.
 *
 * @note 
 * - Ensure PCA9685 boards are initialized with `pca9685_init` before using this function.
 * - The function assumes linear mapping of servo angles to PWM pulse widths.
 */
esp_err_t pca9685_set_angle(pca9685_board_t *controller_data, uint16_t motor_mask,
                            uint8_t board_id, float angle);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_PCA9685_HAL_H */

