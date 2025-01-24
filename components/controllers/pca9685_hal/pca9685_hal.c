/* components/controllers/pca9685_hal/pca9685_hal.c */

#include "pca9685_hal.h"
#include "common/i2c.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

/* Constants ******************************************************************/

const uint8_t    pca9685_scl_io           = GPIO_NUM_22;
const uint8_t    pca9685_sda_io           = GPIO_NUM_21;
const uint32_t   pca9685_i2c_freq_hz      = 100000;
const uint8_t    pca9685_i2c_address      = 0x40;
const i2c_port_t pca9685_i2c_bus          = I2C_NUM_0;
const uint32_t   pca9685_osc_freq         = 25000000; /**< 25MHz internal osc */
const uint16_t   pca9685_pwm_resolution   = 4096;     /**< 12-bit resolution */
const uint16_t   pca9685_default_pwm_freq = 50;       /**< Standard servo freq */
const uint16_t   pca9685_max_pwm_value    = 4095;     /**< 0 to 4095 */
const uint16_t   pca9685_pwm_period_us    = 20000;    /**< 50Hz = 20ms */
const char      *pca9685_tag              = "PCA9685";
const uint8_t    pca9685_step_size_deg    = 5;
const uint32_t   pca9685_step_delay_ms    = 20;
const float      pca9685_default_angle    = 90.0f;

/* Constants (Static) **********************************************************/

static const uint16_t servo_min_pulse_us = 500;  /**< ~0.5ms for 0° */
static const uint16_t servo_max_pulse_us = 2750; /**< ~2.5ms for 180° */

/* Private Functions (Static) *************************************************/

/**
 * @brief Calculate the prescaler value based on the desired PWM frequency.
 *
 * @param[in] pwm_freq Desired PWM frequency in Hz.
 * @return The prescaler value.
 */
static inline uint8_t priv_calculate_prescaler(uint16_t pwm_freq) 
{
  /* Formula from PCA9685 datasheet:
   * prescale = round((osc_clk / (4096 * freq)) - 1) 
   * (the +0.5 accounts for rounding)
   */
  return (uint8_t)(((float)pca9685_osc_freq / (pca9685_pwm_resolution * pwm_freq)) - 1 + 0.5);
}

/**
 * @brief Convert an angle [0°,180°] to a suitable PWM value for the PCA9685.
 *
 * The working code snippet uses a known good mapping:
 * At 0°, the value ~204
 * At 180°, the value ~409
 *
 * This corresponds roughly to 1ms-2ms pulses on a 20ms frame.
 * 
 * Adjust these constants if needed for your servo.
 *
 * @param[in] angle Angle in degrees [0,180].
 * @return The 12-bit pulse length (0–4095).
 */
static inline uint16_t priv_angle_to_pulse_length(float angle)
{
  if (angle < 0.0f) {
    angle = 0.0f;
  } else if (angle > 180.0f) {
    angle = 180.0f;
  }

  float pulse_us = servo_min_pulse_us + (angle / 180.0f) * (servo_max_pulse_us - servo_min_pulse_us);
  
  /* Convert pulse width (us) to PCA9685 steps:
     steps = (pulse_us / 20000us) * 4096 */
  float    steps_f = (pulse_us / (float)pca9685_pwm_period_us) * (float)pca9685_max_pwm_value;
  uint16_t steps   = (uint16_t)(steps_f + 0.5f);
  
  if (steps > pca9685_max_pwm_value) {
    steps = pca9685_max_pwm_value;
  }
  return steps;
}


/* Public Functions ***********************************************************/

esp_err_t pca9685_init(pca9685_board_t **controller_data, uint8_t num_boards)
{
  esp_err_t ret;

  for (uint8_t i = 0; i < num_boards; i++) {
    pca9685_board_t *current = *controller_data;

    /* Check if the board is already initialized */
    while (current != NULL) {
      if (current->board_id == i) {
        ESP_LOGI(pca9685_tag, "PCA9685 board %d already initialized", i);
        break;
      }
      current = current->next;
    }

    /* If the board is already initialized, skip it */
    if (current != NULL) {
      continue;
    }

    /* Allocate memory for the new board */
    pca9685_board_t *new_board = (pca9685_board_t *)malloc(sizeof(pca9685_board_t));
    if (new_board == NULL) {
      ESP_LOGE(pca9685_tag, "Failed to allocate memory for PCA9685 board %d", i);
      return ESP_ERR_NO_MEM;
    }

    /* Initialize the motors array */
    memset(&new_board->motors, 0, sizeof(new_board->motors));
    for (int j = 0; j < PCA9685_MOTORS_PER_BOARD; j++) {
      new_board->motors[j].pos_deg  = pca9685_default_angle; /* Initialize position to default angle */
      new_board->motors[j].board_id = i;
      new_board->motors[j].motor_id = j;
    }

    /* Rest of your initialization code... */
    new_board->i2c_address = pca9685_i2c_address + i;
    new_board->i2c_bus     = pca9685_i2c_bus;

    /* Initialize I2C */
    ret = priv_i2c_init(pca9685_scl_io, pca9685_sda_io,
                        pca9685_i2c_freq_hz,
                        pca9685_i2c_bus, pca9685_tag);
    if (ret != ESP_OK) {
      ESP_LOGE(pca9685_tag, "Failed to initialize I2C for PCA9685 board %d", i);
      free(new_board);
      return ret;
    }

    /* Put the PCA9685 into sleep mode before setting the frequency */
    ret = priv_i2c_write_reg_byte(k_pca9685_mode1_cmd, k_pca9685_sleep_cmd,
                                  pca9685_i2c_bus, new_board->i2c_address, pca9685_tag);
    if (ret != ESP_OK) {
      ESP_LOGE(pca9685_tag, "Failed to put PCA9685 board %d into sleep mode", i);
      free(new_board);
      return ret;
    }

    /* Set the prescaler for the PWM frequency */
    uint8_t prescaler = priv_calculate_prescaler(pca9685_default_pwm_freq);
    ret               = priv_i2c_write_reg_byte(k_pca9685_prescale_cmd, prescaler,
                                                pca9685_i2c_bus, new_board->i2c_address,
                                                pca9685_tag);
    if (ret != ESP_OK) {
      ESP_LOGE(pca9685_tag, "Failed to set prescaler value for PCA9685 board %d", i);
      free(new_board);
      return ret;
    }

    /* Wake up the PCA9685 (restart mode) */
    ret = priv_i2c_write_reg_byte(k_pca9685_mode1_cmd, k_pca9685_restart_cmd,
                                  pca9685_i2c_bus, new_board->i2c_address,
                                  pca9685_tag);
    if (ret != ESP_OK) {
      ESP_LOGE(pca9685_tag, "Failed to restart PCA9685 board %d", i);
      free(new_board);
      return ret;
    }

    /* Set MODE2 to define output logic */
    ret = priv_i2c_write_reg_byte(k_pca9685_mode2_cmd, k_pca9685_output_logic_mode,
                                  pca9685_i2c_bus, new_board->i2c_address,
                                  pca9685_tag);
    if (ret != ESP_OK) {
      ESP_LOGE(pca9685_tag, "Failed to set MODE2 for PCA9685 board %d", i);
      free(new_board);
      return ret;
    }

    /* Set board state and link it into the list */
    new_board->state      = k_pca9685_ready;
    new_board->board_id   = i;
    new_board->num_boards = num_boards;
    new_board->next       = *controller_data;

    /* Set all motors to 90 degrees physically */
    ret = pca9685_set_angle(new_board, 0xFFFF, i, 90.0f);
    if (ret != ESP_OK) {
      ESP_LOGE(pca9685_tag, "Failed to set motors on board %d", i);
      free(new_board);
      return ret;
    }

    *controller_data = new_board;
  }

  return ESP_OK;
}

esp_err_t pca9685_set_angle(pca9685_board_t *controller_data, uint16_t motor_mask,
                            uint8_t board_id, float target_angle)
{
  if (controller_data == NULL) {
    ESP_LOGE(pca9685_tag, "Controller data is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  if (board_id >= controller_data->num_boards) {
    ESP_LOGE(pca9685_tag, "Invalid board_id: %d. Number of boards: %d", board_id,
             controller_data->num_boards);
    return ESP_ERR_INVALID_ARG;
  }

  pca9685_board_t *current_board = controller_data;
  while (current_board != NULL) {
    if (current_board->board_id == board_id) {
      if (current_board->state != k_pca9685_ready) {
        ESP_LOGE(pca9685_tag, "PCA9685 board %d is not ready for communication",
                 current_board->board_id);
        return ESP_FAIL;
      }

      for (uint8_t channel = 0; channel < PCA9685_MOTORS_PER_BOARD; ++channel) {
        if (motor_mask & (1 << channel)) {
          float current_angle = current_board->motors[channel].pos_deg;
          float angle_step    = (target_angle > current_angle) ? 
                                pca9685_step_size_deg : -pca9685_step_size_deg;

          while ((angle_step > 0 && current_angle < target_angle) ||
                 (angle_step < 0 && current_angle > target_angle)) {
            current_angle += angle_step;

            if ((angle_step > 0 && current_angle > target_angle) ||
                (angle_step < 0 && current_angle < target_angle)) {
              current_angle = target_angle;
            }

            uint16_t pulse_length = priv_angle_to_pulse_length(current_angle);

            uint8_t led_on_l_reg  = k_pca9685_channel0_on_l_cmd  + 4 * channel;
            uint8_t led_on_h_reg  = k_pca9685_channel0_on_h_cmd  + 4 * channel;
            uint8_t led_off_l_reg = k_pca9685_channel0_off_l_cmd + 4 * channel;
            uint8_t led_off_h_reg = k_pca9685_channel0_off_h_cmd + 4 * channel;

            esp_err_t ret = priv_i2c_write_reg_byte(led_on_l_reg, 0x00, pca9685_i2c_bus,
                                                    current_board->i2c_address,
                                                    pca9685_tag);
            if (ret != ESP_OK) {
              ESP_LOGE(pca9685_tag, "Failed to set ON_L for motor %d on PCA9685 board %d",
                       channel, current_board->board_id);
              return ret;
            }

            ret = priv_i2c_write_reg_byte(led_on_h_reg, 0x00, pca9685_i2c_bus,
                                          current_board->i2c_address,
                                          pca9685_tag);
            if (ret != ESP_OK) {
              ESP_LOGE(pca9685_tag, "Failed to set ON_H for motor %d on PCA9685 board %d",
                       channel, current_board->board_id);
              return ret;
            }

            ret = priv_i2c_write_reg_byte(led_off_l_reg, pulse_length & 0xFF,
                                          pca9685_i2c_bus, current_board->i2c_address,
                                          pca9685_tag);
            if (ret != ESP_OK) {
              ESP_LOGE(pca9685_tag, "Failed to set OFF_L for motor %d on PCA9685 board %d",
                       channel, current_board->board_id);
              return ret;
            }

            ret = priv_i2c_write_reg_byte(led_off_h_reg, (pulse_length >> 8) & 0xFF,
                                          pca9685_i2c_bus, current_board->i2c_address,
                                          pca9685_tag);
            if (ret != ESP_OK) {
              ESP_LOGE(pca9685_tag, "Failed to set OFF_H for motor %d on PCA9685 board %d",
                       channel, current_board->board_id);
              return ret;
            }

            ESP_LOGD(pca9685_tag, "Moved channel %d on board %d to %.2f°, pulse %u",
                     channel, current_board->board_id, current_angle, pulse_length);

            current_board->motors[channel].pos_deg = current_angle;

            vTaskDelay(pdMS_TO_TICKS(pca9685_step_delay_ms));
          }
        }
      }

      return ESP_OK;
    }
    current_board = current_board->next;
  }

  ESP_LOGE(pca9685_tag, "PCA9685 board with board_id %d not found", board_id);
  return ESP_ERR_NOT_FOUND;
}
