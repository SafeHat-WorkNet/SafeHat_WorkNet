/* main/include/managers/include/time_manager.h */

#ifndef MESHNET_TIME_MANAGER_H
#define MESHNET_TIME_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/* Globals (Constants) ********************************************************/

extern const char *time_manager_tag; /**< For Logging */

/* Public Functions ***********************************************************/

/**
 * @brief Initializes the SNTP service and synchronizes the ESP32's system time.
 *
 * Sets up the Simple Network Time Protocol (SNTP) service to retrieve the 
 * current date and time from an NTP server. If synchronization with the NTP 
 * server fails (e.g., due to lack of Wi-Fi connectivity), the system falls 
 * back to manually setting the time to a predefined default value ("beginning 
 * of time"). The synchronized system time is critical for logging and 
 * timestamping operations across the application.
 *
 * @return
 * - ESP_OK   if the system time is successfully synchronized with an NTP server.
 * - ESP_FAIL if synchronization fails and the default fallback time is set.
 *
 * @note
 * - Call this function once during the system initialization phase.
 * - Ensure Wi-Fi connectivity is established before invoking this function 
 *   for successful NTP synchronization.
 */
esp_err_t time_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_TIME_MANAGER_H */

