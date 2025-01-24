/* main/include/gait_movement.h */

#ifndef TOPOROBO_GAIT_MOVEMENT_H
#define TOPOROBO_GAIT_MOVEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "pca9685_hal.h"

/* Constants ******************************************************************/

extern const char *gait_tag; /**< Tag for logs */

/* Public Functions ***********************************************************/

/**
 * @brief Initializes the legs array for the hexapod robot.
 *
 * Sets up the static array of legs, defining each leg's servo mappings, joint 
 * types, and physical constraints. Ensures the robot's gait logic references 
 * consistent and correctly configured data for controlling the legs during motion. 
 * This must be called after running `motor_init` or initializing the PWM controller manually.
 *
 * @param[in] pwm_controller Pointer to the PCA9685 board controller used to 
 *                           drive the servos for all legs.
 *
 * @return ESP_OK on successful initialization, or ESP_FAIL if the initialization
 *         fails due to invalid configurations or other errors.
 *
 * @note This function is not thread-safe and should be called during the 
 *       initialization phase of the robot's setup.
 */
esp_err_t gait_init(pca9685_board_t *pwm_controller);

/**
 * @brief Executes a tripod gait motion for the hexapod robot.
 *
 * Moves three legs simultaneously to achieve smooth and fast locomotion. 
 * Suitable for high-speed traversal with moderate stability.
 *
 * @param[in] pwm_controller Pointer to the PCA9685 board controller.
 * @param[in] heading        Desired heading in degrees (0-360).
 * @param[in] distance       Distance to be traveled in centimeters.
 *
 * @return ESP_OK on success, ESP_FAIL if motion execution fails.
 *
 * @note Ensure `gait_init` has been called before invoking this function.
 */
esp_err_t tripod_gait(pca9685_board_t *pwm_controller, float heading, 
                      uint16_t distance);

/**
 * @brief Executes a wave gait motion for the hexapod robot.
 *
 * Moves one leg at a time, maintaining maximum stability. This gait is slower 
 * but ideal for uneven or unstable terrains.
 *
 * @param[in] pwm_controller Pointer to the PCA9685 board controller.
 * @param[in] heading        Desired heading in degrees (0-360).
 * @param[in] distance       Distance to be traveled in centimeters.
 *
 * @return ESP_OK on success, ESP_FAIL if motion execution fails.
 *
 * @note Ensure `gait_init` has been called before invoking this function.
 */
esp_err_t wave_gait(pca9685_board_t *pwm_controller, float heading, 
                    uint16_t distance);

/**
 * @brief Executes a ripple gait motion for the hexapod robot.
 *
 * Moves two legs at a time to maintain balance while achieving moderate speed. 
 * Suitable for scenarios requiring both stability and efficiency.
 *
 * @param[in] pwm_controller Pointer to the PCA9685 board controller.
 * @param[in] heading        Desired heading in degrees (0-360).
 * @param[in] distance       Distance to be traveled in centimeters.
 *
 * @return ESP_OK on success, ESP_FAIL if motion execution fails.
 *
 * @note Ensure `gait_init` has been called before invoking this function.
 */
esp_err_t ripple_gait(pca9685_board_t *pwm_controller, float heading, 
                      uint16_t distance);

/**
 * @brief Executes a quadruped gait motion for the hexapod robot.
 *
 * Uses four legs to walk, leaving two stationary for enhanced stability. This 
 * gait is suitable for heavy loads or when the highest possible stability is 
 * required.
 *
 * @param[in] pwm_controller Pointer to the PCA9685 board controller.
 * @param[in] heading        Desired heading in degrees (0-360).
 * @param[in] distance       Distance to be traveled in centimeters.
 *
 * @return ESP_OK on success, ESP_FAIL if motion execution fails.
 *
 * @note Ensure `gait_init` has been called before invoking this function.
 */
esp_err_t quadruped_gait(pca9685_board_t *pwm_controller, float heading, 
                         uint16_t distance);

#ifdef __cplusplus
}
#endif

#endif /* TOPOROBO_GAIT_MOVEMENT_H */

