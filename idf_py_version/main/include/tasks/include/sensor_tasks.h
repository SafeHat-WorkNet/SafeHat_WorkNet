/* main/include/tasks/include/sensor_tasks.h */

#ifndef MESHNET_SENSOR_TASKS_H
#define MESHNET_SENSOR_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor_hal.h"
#include "esp_err.h"
#include "portmacro.h"

/* Structs ********************************************************************/

/**
 * @brief Structure to hold configuration for each sensor.
 *
 * Represents a sensor's configuration, including its metadata, initialization 
 * and task functions, data pointer, and an enablement flag.
 */
typedef struct {
  const char *sensor_name;            /**< Sensor name used for identification in logs and debugging. */
  esp_err_t (*init_function)(void *); /**< Pointer to the function that initializes the sensor. */
  void      (*task_function)(void *); /**< Pointer to the function that handles the sensor's tasks. */
  void       *data_ptr;               /**< Pointer to the structure holding sensor-specific data. */
  UBaseType_t priority;               /**< Priority of the sensor's task for scheduling purposes. */
  uint32_t    stack_depth;            /**< Stack depth allocated for the sensor task, in words. */
  bool        enabled;                /**< Flag indicating if the sensor is enabled (true) or disabled (false). */
} sensor_config_t;

/* Public Functions ***********************************************************/

/**
 * @brief Initializes communication with various sensors.
 *
 * Establishes communication with all configured sensors using I2C, UART, or other
 * protocols as required. This function ensures that each sensor is properly
 * initialized and ready for data acquisition. Any sensor that fails initialization
 * will be logged, and the function will return an error.
 *
 * @param[out] sensor_data Pointer to the `sensor_data_t` struct where sensor readings 
 *                         will be stored. This structure is initialized as part of the
 *                         function's execution.
 *
 * @return 
 * - ESP_OK   if all sensors initialize successfully.
 * - ESP_FAIL if any sensor fails to initialize.
 *
 * @note Ensure that the underlying communication protocols (e.g., I2C and UART) are 
 *       initialized before calling this function.
 */
esp_err_t sensors_init(sensor_data_t *sensor_data);

/**
 * @brief Records sensor data and stores it in a given variable.
 *
 * Continuously records data from all configured sensors and updates the provided
 * `sensor_data_t` structure with the latest readings. This function processes the
 * raw data from sensors and ensures it is ready for further use, such as uploading
 * to a server or storing in a database. The function relies on previously 
 * established sensor communication initialized by `sensors_init`.
 *
 * Pre-condition:
 * - The `sensors_init` function must have been successfully called and completed 
 *   to ensure communication with all sensors is established.
 *
 * Post-condition:
 * - The `sensor_data_t` structure is updated with the most recent sensor readings.
 * - The data is available for further processing or transmission.
 *
 * @param[in,out] sensor_data Pointer to the `sensor_data_t` struct where sensor 
 *                            readings will be stored and updated.
 *
 * @return 
 * - ESP_OK   if all sensor tasks are successfully started and run.
 * - ESP_FAIL if any sensor task fails during execution.
 *
 * @note This function should run continuously as part of the main sensor task loop 
 *       or be called periodically in a task for data acquisition.
 */
esp_err_t sensor_tasks(sensor_data_t *sensor_data);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_SENSOR_TASKS_H */

