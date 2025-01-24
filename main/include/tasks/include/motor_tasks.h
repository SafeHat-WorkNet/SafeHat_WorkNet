/* main/include/tasks/include/motor_tasks.h */

#ifndef MESHNET_MOTOR_TASKS_H
#define MESHNET_MOTOR_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pca9685_hal.h"
#include "esp_err.h"

/* Constants ******************************************************************/

extern const uint8_t num_pca9685_boards; /**< The number of boards used in this project */
extern const char   *motor_tag;          /**< Tag for logs */

/* Public Functions ***********************************************************/

/**
 * @brief Initializes the motor tasks.
 *
 * Configures and initializes motor control tasks using the provided linked list 
 * of PCA9685 board controllers. This function prepares the controllers for 
 * operation but does not start the tasks; call `motor_tasks_start` to begin 
 * execution.
 *
 * @param[in] pwm_controller Pointer to the linked list of PCA9685 board 
 *                           controllers managing the motors.
 *
 * @return 
 * - ESP_OK              on successful initialization.
 * - ESP_ERR_INVALID_ARG if the `pwm_controller` is NULL.
 * - ESP_FAIL            for other initialization errors.
 *
 * @note Ensure that the PCA9685 controllers are properly initialized before 
 *       invoking this function.
 */
esp_err_t motors_init(pca9685_board_t **pwm_controller);

/**
 * @brief Starts the motor tasks.
 *
 * Activates motor control tasks to manage motor operations using the provided 
 * PCA9685 board controllers. This function should be called after 
 * `motors_init` to ensure all motor tasks are ready for execution.
 *
 * @param[in] pwm_controller Pointer to the linked list of PCA9685 board 
 *                           controllers managing the motors.
 *
 * @return 
 * - ESP_OK              on successful start of motor tasks.
 * - ESP_ERR_INVALID_ARG if the `pwm_controller` is NULL.
 * - ESP_FAIL            for other runtime errors.
 *
 * @note Call this function only after successfully initializing the motor tasks 
 *       using `motors_init`.
 */
esp_err_t motor_tasks_start(pca9685_board_t *pwm_controller);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_MOTOR_TASKS_H */

