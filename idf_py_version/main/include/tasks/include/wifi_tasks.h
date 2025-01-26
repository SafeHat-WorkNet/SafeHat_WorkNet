/* main/include/tasks/include/wifi_tasks.h */

#ifndef MESHNET_WIFI_TASKS_H
#define MESHNET_WIFI_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: check wifi_credentials.txt is included, but wifi_credentials.h isn't
 * copy wifi_credentials.txt to wifi_credentials.h and replace the values */
#include <stdbool.h>
#include "wifi_credentials.h"
#include "esp_err.h"

/* Constants ******************************************************************/

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT (BIT0) /**< Wifi is Connected */
#define WIFI_FAIL_BIT      (BIT1) /**< Wifi failed to connect */

extern const char    *wifi_tag;                /**< Tag for logging */
extern const uint8_t  wifi_max_retry;          /**< The max number of times to try and connect to the station */
extern const uint8_t  wifi_ssid_max_len;       /**< The max length for wifi's SSID defined by esp */
extern const uint8_t  wifi_pass_max_len;       /**< The max length for wifi's password defined by esp */
extern const uint32_t wifi_connect_timeout_ms; /**< Timeout for WiFi connection in milliseconds */

/* Public Functions ***********************************************************/

/**
 * @brief Checks if the system has a valid IP address and can reach the internet.
 *
 * Verifies the current network status by checking if the WiFi station (STA)
 * interface has a valid IP address assigned. This function can be used to
 * confirm internet connectivity before attempting network operations.
 *
 * @return 
 * - ESP_OK   if the network is up and a valid IP address is assigned.
 * - ESP_FAIL if the interface does not exist or no IP address is assigned.
 *
 * @note 
 * - Ensure that WiFi is initialized and connected to an Access Point (AP) 
 *   before calling this function.
 * - This function does not test actual internet connectivity, only the 
 *   presence of a valid IP address.
 */
esp_err_t wifi_check_connection(void);

/**
 * @brief Initializes the WiFi in station mode and connects to the specified Access Point (AP).
 *
 * Sets up the ESP32's WiFi system to operate in station (STA) mode and attempts
 * to connect to a preconfigure Access Point (AP). The initialization process
 * includes setting up the required event handlers, configuring the network
 * interface, and managing the connection lifecycle. Upon successful connection,
 * the system acquires an IP address for further network operations.
 *
 * Steps performed:
 *   1. Creates an event group to manage WiFi events.
 *   2. Initializes the underlying TCP/IP stack.
 *   3. Creates the default event loop required for handling WiFi and IP events.
 *   4. Sets up the default WiFi station (STA) network interface.
 *   5. Initializes the WiFi driver with default configuration settings.
 *   6. Registers event handlers for WiFi and IP events.
 *   7. Configures the WiFi settings with the specified SSID, password, 
 *      authentication mode, and SAE settings if applicable.
 *   8. Sets the WiFi mode to station (STA) mode.
 *   9. Starts the WiFi driver.
 *   10. Waits for the WiFi connection to be established or logs an error on failure.
 *
 * @return 
 * - ESP_OK   if the WiFi is initialized and connected successfully.
 * - ESP_FAIL if any step in the initialization or connection process fails.
 *
 * @note 
 * - Ensure the SSID and password are correctly configured before calling this function.
 * - This function blocks until a connection is established or a failure occurs.
 * - Check the logs for detailed information about the connection status and errors.
 */
esp_err_t wifi_init_sta(void);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_WIFI_TASKS_H */
