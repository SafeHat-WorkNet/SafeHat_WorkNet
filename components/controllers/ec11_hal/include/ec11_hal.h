/* components/controllers/ec11_hal/include/ec11_hal.h */

#ifndef MESHNET_EC11_HAL_H
#define MESHNET_EC11_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* Configuration Options ****************************************************/

#ifndef EC11_USE_INTERRUPTS
#define EC11_USE_INTERRUPTS 0 /**< Set to 1 to use interrupts, 0 for polling */
#endif

/* Constants ******************************************************************/

extern const char    *ec11_tag;      /**< Tag for logging messages related to the EC11 encoder. */
extern const uint32_t poll_delay_ms; /**< Delay between encoder state polls in milliseconds. */

/* Enums **********************************************************************/

/**
 * @brief Enumeration of EC11 encoder states.
 *
 * Represents the possible states of the EC11 rotary encoder based on its output pins.
 */
typedef enum : uint8_t {
  k_ec11_state_00 = 0b00, /**< Both outputs low */
  k_ec11_state_01 = 0b01, /**< Output A low, B high */
  k_ec11_state_11 = 0b11, /**< Both outputs high */
  k_ec11_state_10 = 0b10, /**< Output A high, B low */
} ec11_states_t;

/**
 * @brief Enumeration of EC11 encoder events.
 *
 * Defines the possible events that can be triggered by the encoder,
 * including rotation direction and button press/release.
 */
typedef enum : uint8_t {
  k_ec11_cw,          /**< Clockwise rotation detected */
  k_ec11_ccw,         /**< Counter-clockwise rotation detected */
  k_ec11_btn_press,   /**< Button press detected */
  k_ec11_btn_release, /**< Button release detected */
} ec11_event_t;

/**
 * @brief Function pointer type for EC11 event callbacks.
 *
 * @param[in] event      The type of event that occurred
 * @param[in] board      Pointer to the PCA9685 board
 * @param[in] motor_mask Bitmask indicating which motor to control
 */
typedef void (*ec11_callback_t)(ec11_event_t event, void *board, uint16_t motor_mask);

/**
 * @brief Enumeration of EC11 encoder operational states.
 *
 * Defines the possible operational states of the EC11 encoder,
 * including initialization status and error conditions.
 */
typedef enum : uint8_t {
  k_ec11_ready         = 0x00, /**< Encoder is initialized and ready */
  k_ec11_uninitialized = 0x10, /**< Encoder is not initialized */
  k_ec11_error         = 0xF0, /**< General error state */
} ec11_op_states_t;

/* Structs ********************************************************************/

/**
 * @brief Structure to store EC11 encoder data and configuration.
 *
 * Contains GPIO pin assignments, current state, position tracking,
 * and button state for a single EC11 encoder instance.
 */
typedef struct {
  gpio_num_t        pin_a;          /**< GPIO pin for encoder output A */
  gpio_num_t        pin_b;          /**< GPIO pin for encoder output B */
  gpio_num_t        pin_btn;        /**< GPIO pin for encoder push button */
  int32_t           position;       /**< Current encoder position (increments/decrements) */
  uint8_t           state;          /**< Current operational state (see ec11_op_states_t) */
  bool              button_pressed; /**< Current state of the push button */
  uint8_t           prev_state;     /**< Previous encoder state for rotation detection */
  ec11_callback_t   callback;       /**< Event callback function */
  void             *board_ptr;      /**< Pointer to the PCA9685 board */
  uint16_t          motor_mask;     /**< Bitmask indicating which motor to control */
#if EC11_USE_INTERRUPTS
  SemaphoreHandle_t mutex;          /**< Mutex for thread-safe access in ISR */
#endif
} ec11_data_t;

/* Public Functions ***********************************************************/

/**
 * @brief Initializes an EC11 rotary encoder instance.
 *
 * Configures the GPIO pins for the encoder's outputs and button.
 * When using interrupts (EC11_USE_INTERRUPTS=1), it sets up GPIO interrupts.
 * When using polling (EC11_USE_INTERRUPTS=0), it configures for polling mode.
 *
 * @param[in] encoder Pointer to the encoder data structure
 * @return
 * - ESP_OK on success
 * - ESP_FAIL or other error codes on failure
 */
esp_err_t ec11_init(ec11_data_t *encoder);

#if EC11_USE_INTERRUPTS
/**
 * @brief ISR handler for EC11 encoder interrupts.
 * This function is automatically registered when using interrupt mode.
 * @param[in] arg Pointer to the encoder data structure
 */
void IRAM_ATTR ec11_isr_handler(void* arg);
#else
/**
 * @brief Task function for polling the EC11 encoder state.
 * Only available in polling mode (EC11_USE_INTERRUPTS=0).
 * @param[in] arg Pointer to the encoder data structure
 */
void ec11_task(void *arg);
#endif

/**
 * @brief Registers a callback function for encoder events.
 *
 * @param[in] encoder  Pointer to the encoder data structure
 * @param[in] callback The callback function to register
 * @param[in] board_ptr Pointer to the PCA9685 board
 * @param[in] motor_mask Bitmask indicating which motor to control
 */
void ec11_register_callback(ec11_data_t *encoder, 
                            ec11_callback_t callback, 
                            void *board_ptr, 
                            uint16_t motor_mask);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_EC11_HAL_H */
