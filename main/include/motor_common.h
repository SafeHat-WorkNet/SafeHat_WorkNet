/* main/include/motor.h */

#ifndef MESHNET_MOTOR_H
#define MESHNET_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "ec11_hal.h"

/* Structs ********************************************************************/

/**
 * @brief Represents a motor associated with a specific joint.
 *
 * Contains information about a motor's type, current position, and identification
 * on the PCA9685 servo controller boards.
 */
typedef struct {
  float       pos_deg;   /**< Current position of the motor in degrees (range: 0 to 180). */
  uint8_t     board_id;  /**< ID of the PCA9685 board (e.g., 0 or 1). */
  uint8_t     motor_id;  /**< ID of the motor on the board (range: 0 to 15). */
  ec11_data_t ec11_data; /**< Data for the EC11 encoder (if applicable). */
} motor_t;

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_MOTOR_H */

