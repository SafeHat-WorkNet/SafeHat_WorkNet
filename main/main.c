/* main/main.c */

#include "system_tasks.h"
#include "esp_log.h"
#include "dotenv.h"

#include "wifi_credentials.h"

void app_main(void)
{
  /* Load environment variables from .env file */
  dotenv_load(".env");

  /* Access the environment variables */
  const char* wifi_ssid = getenv("WIFI_SSID");
  const char* wifi_pass = getenv("WIFI_PASS");

  /* Use the environment variables */
  printf("WiFi SSID: %s\n", wifi_ssid);
  printf("WiFi Password: %s\n", wifi_pass);

  /* Load Wi-Fi credentials from .env file */
  load_wifi_credentials();

  /* Initialize System-Level Tasks (motor, sensors, webserver, etc) */
  if (system_tasks_init() != ESP_OK) {
    ESP_LOGE(system_tag, "System tasks initialization failed.");
  } else {
    ESP_LOGI(system_tag, "System tasks initialized successfully.");
  }

  /* Start System-Level Tasks (motor, sensors, webserver, etc) */
  if (system_tasks_start() != ESP_OK) {
    ESP_LOGE(system_tag, "Failed to start system tasks. Exiting.");
    return; /* Exit app_main if tasks cannot start */
  } else {
    ESP_LOGI(system_tag, "System tasks started successfully.");
  }

  ESP_LOGI(system_tag, "Meshnet system initialized and running.");
}

