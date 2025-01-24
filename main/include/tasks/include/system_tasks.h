/* main/include/tasks/include/system_tasks.h */

#ifndef MESHNET_SYSTEM_TASKS_H
#define MESHNET_SYSTEM_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "sensor_tasks.h"
#include "wifi_tasks.h"
#include "motor_tasks.h"
#include "pca9685_hal.h"
#include "ov7670_hal.h"

/* Constants ******************************************************************/

extern const char *system_tag; /**< Logging tag */

/* Globals ********************************************************************/

extern sensor_data_t    g_sensor_data;    /**< Global variable that holds the sensor data */
extern pca9685_board_t *g_pwm_controller; /**< Global variable that holds the PWM controller linked list */
extern ov7670_data_t    g_camera_data;    /**< Global variable that holds the camera data */

/* Public Functions ***********************************************************/

/**
 * @brief Initializes system-level tasks for handling devices and communication.
 *
 * Prepares system tasks required for managing hardware devices and communication 
 * subsystems. This includes setting up resources for motor monitoring, sensor 
 * data collection, video streaming to the web server, and Wi-Fi operations. 
 * Initialization ensures that each task has the necessary prerequisites to start.
 *
 * @return 
 * - ESP_OK   if all system-level tasks are initialized successfully.
 * - ESP_FAIL if any initialization step fails.
 *
 * @note This function should be called once during system setup, prior to starting 
 *       the tasks with `system_tasks_start`.
 */
esp_err_t system_tasks_init(void);

/**
 * @brief Starts system-level tasks for handling devices and communication.
 *
 * Creates and starts tasks responsible for the following functions:
 * - Monitoring and controlling motors.
 * - Collecting and processing sensor data.
 * - Relaying video streams to the web server.
 * - Managing Wi-Fi connectivity and related operations.
 *
 * Tasks are pinned to the appropriate ESP32 cores to optimize performance and 
 * ensure efficient resource utilization:
 * - Core 0: Wi-Fi handling.
 * - Core 1: Motor monitoring, sensor data collection, and webserver video relay.
 *
 * @return 
 * - ESP_OK   if all tasks start successfully.
 * - ESP_FAIL if any task fails to start.
 *
 * @note Ensure that `system_tasks_init` has been called successfully before invoking 
 *       this function. Task creation may fail if system resources are insufficient 
 *       or prerequisites are unmet.
 */
esp_err_t system_tasks_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_SYSTEM_TASKS_H */

