/* main/include/tasks/motor_tasks.c */

#include "motor_tasks.h"
#include "pca9685_hal.h"
#include "ec11_hal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Constants ******************************************************************/

const uint8_t num_pca9685_boards = 1;
const char   *motor_tag          = "Motor Tasks";

/* Private Functions (Static) *************************************************/

/**
 * @brief Callback function for EC11 encoder events.
 *
 * Handles encoder rotation and button events for manual motor control.
 * Clockwise rotation increases angle, counterclockwise decreases angle,
 * and button press resets motor to default position.
 *
 * @param[in] event      The encoder event (CW, CCW, or button press)
 * @param[in] arg      Pointer to the PCA9685 board
 * @param[in] motor_mask Bitmask indicating which motor to control
 */ 
static void priv_motor_control_callback(ec11_event_t event, 
                                      void *arg, 
                                      uint16_t motor_mask)
{
  static const float angle_step = 5.0f; /* 5 degrees per detent */
  pca9685_board_t   *board      = (pca9685_board_t *)arg;

  /* Iterate through all possible motors in the mask */
  for (uint8_t i = 0; i < PCA9685_MOTORS_PER_BOARD; i++) {
    /* Check if this motor is selected in the mask */ 
    if (motor_mask & (1 << i)) {
      switch (event) {
      case k_ec11_cw:
        if (board->motors[i].pos_deg + angle_step <= 180.0f) {
          pca9685_set_angle(board, (1 << i), board->board_id, 
                            board->motors[i].pos_deg + angle_step);
        }
        break;

      case k_ec11_ccw:
        if (board->motors[i].pos_deg - angle_step >= 0.0f) {
          pca9685_set_angle(board, (1 << i), board->board_id, 
                            board->motors[i].pos_deg - angle_step);
        }
        break;

      case k_ec11_btn_press:
        pca9685_set_angle(board, (1 << i), board->board_id, 
                          pca9685_default_angle);
        break;

      case k_ec11_btn_release: /* Do nothing (Fall through) */
      default:                 /* Do nothing */
      }
    }
  }
}

/* Public Functions ***********************************************************/

esp_err_t motors_init(pca9685_board_t **pwm_controller)
{
  esp_err_t ret = pca9685_init(pwm_controller, num_pca9685_boards);
  if (ret != ESP_OK) {
    ESP_LOGE(motor_tag, "Failed to initialize all PWM boards.");
    return ret;
  }
  ESP_LOGI(motor_tag, "Initialized all PWM boards with default values.");

  /* TODO: Initialize the EC11 encoders */

  /* Register the motor control callback for each motor */
  for (int i = 0; i < num_pca9685_boards; i++) {
    for (int j = 0; j < PCA9685_MOTORS_PER_BOARD; j++) {
      pca9685_board_t *board = &(*pwm_controller)[i];
      ec11_register_callback(&board->motors[j].ec11_data, 
                             priv_motor_control_callback, 
                             board, 
                             j);
    }
  }

  return ESP_OK;
}

esp_err_t motor_tasks_start(pca9685_board_t *pwm_controller)
{
  ESP_LOGI(motor_tag, "Started priv_testing task.");

  /* Example Usage -- we still need to implement this so it can be used
   * either by like a controller, webui/app (webserver), etc
   *
   * xTaskCreate(...)
   * func(...):
   *  while (1) {
   *    func *gait_type = select_gait_func(some_input_val);
   *    gait_type();
   *    xTaskDelay(...);
   *  }
   */

  return ESP_OK;
}
