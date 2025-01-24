/* main/include/tasks/include/webserver_tasks.h */

#ifndef MESHNET_WEBSERVER_TASKS_H
#define MESHNET_WEBSERVER_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/* Public Functions ***********************************************************/

/**
 * @brief Sends a JSON string containing sensor data to the web server.
 *
 * Transmits a JSON-formatted string to a pre-configured web server endpoint
 * using HTTP POST. This function handles the underlying network communication
 * and ensures that the data is successfully sent. In case of a failure, the
 * error code provides details about the issue.
 *
 * @param[in] json_string Pointer to the null-terminated JSON string to be sent. 
 *                        The string must be properly formatted and must not 
 *                        exceed the maximum payload size supported by the 
 *                        network library.
 *
 * @return 
 * - ESP_OK              if the data is sent successfully.
 * - ESP_ERR_INVALID_ARG if `json_string` is NULL or improperly formatted.
 * - ESP_FAIL            if a network or transmission error occurs.
 * - Other error codes as defined by the network stack.
 *
 * @note 
 * - Ensure that the device is connected to the network before calling this function.
 * - The function does not perform retries; consider implementing retry logic if needed.
 * - The server endpoint and configuration (e.g., URL, port) must be predefined 
 *   in the application.
 */
esp_err_t send_sensor_data_to_webserver(const char *json_string);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_WEBSERVER_TASKS_H */
