/* components/sensors/ccs811_hal/ccs811_hal.c */

/* TODO: Add Wake, Reset, and Interrupt GPIOs */

#include "ccs811_hal.h"
#include "file_write_manager.h"
#include "webserver_tasks.h"
#include "cJSON.h"
#include "common/i2c.h"
#include "esp_log.h"
#include "error_handler.h"
#include "driver/gpio.h"

/* Constants ******************************************************************/

const uint8_t    ccs811_i2c_address            = 0x5A;
const i2c_port_t ccs811_i2c_bus                = I2C_NUM_0;
const char      *ccs811_tag                    = "CCS811";
const uint8_t    ccs811_scl_io                 = GPIO_NUM_22;
const uint8_t    ccs811_sda_io                 = GPIO_NUM_21;
const uint32_t   ccs811_i2c_freq_hz            = 100000;
const uint32_t   ccs811_polling_rate_ticks     = pdMS_TO_TICKS(1 * 1000);
const uint8_t    ccs811_max_retries            = 4;
const uint32_t   ccs811_initial_retry_interval = pdMS_TO_TICKS(15 * 1000);
const uint32_t   ccs811_max_backoff_interval   = pdMS_TO_TICKS(8 * 60 * 1000);
const uint8_t    ccs811_allowed_fail_attempts  = 3;

/* Public Functions ***********************************************************/

char *ccs811_data_to_json(const ccs811_data_t *data)
{
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    ESP_LOGE(ccs811_tag, "Failed to create JSON object.");
    return NULL;
  }

  if (!cJSON_AddStringToObject(json, "sensor_type", "air_quality")) {
    ESP_LOGE(ccs811_tag, "Failed to add sensor_type to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "eCO2", data->eco2)) {
    ESP_LOGE(ccs811_tag, "Failed to add eCO2 to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "TVOC", data->tvoc)) {
    ESP_LOGE(ccs811_tag, "Failed to add TVOC to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  char *json_string = cJSON_PrintUnformatted(json);
  if (!json_string) {
    ESP_LOGE(ccs811_tag, "Failed to serialize JSON object.");
    cJSON_Delete(json);
    return NULL;
  }

  cJSON_Delete(json);
  return json_string;
}

esp_err_t ccs811_init(void *sensor_data)
{
  ccs811_data_t *data = (ccs811_data_t *)sensor_data;
  ESP_LOGI(ccs811_tag, "Starting CCS811 Configuration");

  /* Initialize data structure */
  data->i2c_address = ccs811_i2c_address;
  data->i2c_bus = ccs811_i2c_bus;
  data->eco2 = 0;
  data->tvoc = 0;
  data->state = k_ccs811_uninitialized;

  /* Initialize error handler */
  error_handler_init(&data->error_handler,
                     ccs811_tag,
                     ccs811_allowed_fail_attempts,
                     ccs811_max_retries,
                     ccs811_initial_retry_interval,
                     ccs811_max_backoff_interval);

  /* Initialize I2C interface */
  esp_err_t ret = priv_i2c_init(ccs811_scl_io, ccs811_sda_io, ccs811_i2c_freq_hz,
                                ccs811_i2c_bus, ccs811_tag);
  if (ret != ESP_OK) {
    data->state = k_ccs811_error;
    return ret;
  }

  /* Start the application */
  ret = priv_i2c_write_byte(k_ccs811_cmd_app_start, ccs811_i2c_bus,
                           ccs811_i2c_address, ccs811_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(ccs811_tag, "Failed to start application: %s", esp_err_to_name(ret));
    data->state = k_ccs811_app_start_error;
    return ret;
  }

  /* Wait for the device to be ready */
  vTaskDelay(pdMS_TO_TICKS(20));

  data->state = k_ccs811_ready;
  ESP_LOGI(ccs811_tag, "CCS811 Configuration Complete");
  return ESP_OK;
}

esp_err_t ccs811_read(ccs811_data_t *sensor_data)
{
  esp_err_t ret;
  uint8_t data[k_ccs811_alg_data_len];

  /* Read the algorithm results */
  ret = priv_i2c_read_reg_bytes(k_ccs811_reg_alg_result_data, data,
                               k_ccs811_alg_data_len, sensor_data->i2c_bus,
                               sensor_data->i2c_address, ccs811_tag);
  
  if (ret != ESP_OK) {
    ESP_LOGE(ccs811_tag, "Failed to read sensor data: %s", esp_err_to_name(ret));
    sensor_data->state = k_ccs811_read_error;
    return ret;
  }

  /* Parse the data */
  sensor_data->eco2 = (data[0] << 8) | data[1];
  sensor_data->tvoc = (data[2] << 8) | data[3];
  sensor_data->state = k_ccs811_data_updated;

  ESP_LOGI(ccs811_tag, "Read successful - eCO2: %d ppm, TVOC: %d ppb",
           sensor_data->eco2, sensor_data->tvoc);
  return ESP_OK;
}

void ccs811_tasks(void *sensor_data)
{
  ccs811_data_t *ccs811_data = (ccs811_data_t *)sensor_data;

  while (1) {
    if (ccs811_read(ccs811_data) == ESP_OK) {
      char *json = ccs811_data_to_json(ccs811_data);
      if (json) {
        send_sensor_data_to_webserver(json);
        file_write_enqueue("ccs811.txt", json);
        free(json);
      }
      ccs811_data->error_handler.fail_count = 0;
    } else {
      ccs811_data->error_handler.fail_count++;
      error_handler_reset(&ccs811_data->error_handler,
                          ccs811_data->error_handler.fail_count,
                          ccs811_init,
                          ccs811_data);
    }
    
    vTaskDelay(ccs811_polling_rate_ticks);
  }
}

