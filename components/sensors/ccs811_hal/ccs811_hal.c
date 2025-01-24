/* components/sensors/ccs811_hal/ccs811_hal.c */

#include "ccs811_hal.h"
#include "file_write_manager.h"
#include "webserver_tasks.h"
#include "cJSON.h"
#include "common/i2c.h"
#include "esp_log.h"
#include "error_handler.h"

/* Constants ******************************************************************/

const uint8_t    ccs811_i2c_address            = 0x5A;
const i2c_port_t ccs811_i2c_bus                = I2C_NUM_0;
const char      *ccs811_tag                    = "CCS811";
const uint8_t    ccs811_scl_io                 = GPIO_NUM_22;
const uint8_t    ccs811_sda_io                 = GPIO_NUM_21;
const uint8_t    ccs811_wake_io                = GPIO_NUM_33;
const uint8_t    ccs811_rst_io                 = GPIO_NUM_32;
const uint8_t    ccs811_int_io                 = GPIO_NUM_25;
const uint32_t   ccs811_i2c_freq_hz            = 100000;
const uint32_t   ccs811_polling_rate_ticks     = pdMS_TO_TICKS(1 * 1000);
const uint8_t    ccs811_max_retries            = 4;
const uint32_t   ccs811_initial_retry_interval = pdMS_TO_TICKS(15 * 1000);
const uint32_t   ccs811_max_backoff_interval   = pdMS_TO_TICKS(8 * 60 * 1000);

/* Globals (Static) ***********************************************************/

static error_handler_t s_ccs811_error_handler = { 0 };

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
  ccs811_data_t *ccs811_data = (ccs811_data_t *)sensor_data;
  ESP_LOGI(ccs811_tag, "Starting Configuration");

  /* TODO: Initialize error handler */

  ccs811_data->i2c_address        = ccs811_i2c_address;
  ccs811_data->i2c_bus            = ccs811_i2c_bus;
  ccs811_data->eco2               = 0;
  ccs811_data->tvoc               = 0;
  ccs811_data->state              = k_ccs811_uninitialized;
  ccs811_data->retry_count        = 0;
  ccs811_data->retry_interval     = ccs811_initial_retry_interval;
  ccs811_data->last_attempt_ticks = 0;

  /* Initialize the I2C bus */
  esp_err_t ret = priv_i2c_init(ccs811_scl_io, ccs811_sda_io, ccs811_i2c_freq_hz,
                                ccs811_i2c_bus, ccs811_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(ccs811_tag, "I2C driver install failed: %s", esp_err_to_name(ret));
    return ret;
  }

  /* Reset the sensor */
  gpio_set_level(ccs811_rst_io, k_ccs811_gpio_low);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(ccs811_rst_io, k_ccs811_gpio_high);
  vTaskDelay(pdMS_TO_TICKS(10));

  /* Wake up the sensor */
  gpio_set_level(ccs811_wake_io, k_ccs811_gpio_low);
  vTaskDelay(pdMS_TO_TICKS(10));

  /* Application start */
  ret = priv_i2c_write_byte(k_ccs811_cmd_app_start, ccs811_i2c_bus, ccs811_i2c_address, 
                            ccs811_tag);
  if (ret != ESP_OK) {
    ccs811_data->state = k_ccs811_app_start_error;
    ESP_LOGE(ccs811_tag, "CCS811 App Start failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ccs811_data->state = k_ccs811_ready;
  ESP_LOGI(ccs811_tag, "CCS811 Configuration Complete");
  return ESP_OK;
}

esp_err_t ccs811_read(ccs811_data_t *sensor_data)
{
  uint8_t data[k_ccs811_alg_data_len];
  esp_err_t ret = priv_i2c_read_bytes(data, k_ccs811_alg_data_len, ccs811_i2c_bus, ccs811_i2c_address,
                                      ccs811_tag);
  if (ret != ESP_OK) {
    sensor_data->eco2  = 0;
    sensor_data->tvoc  = 0;
    sensor_data->state = k_ccs811_read_error;
    ESP_LOGE(ccs811_tag, "Failed to read data from CCS811");
    return ESP_FAIL;
  }

  sensor_data->eco2 = (data[0] << 8) | data[1];
  sensor_data->tvoc = (data[2] << 8) | data[3];
  ESP_LOGI(ccs811_tag, "eCO2: %d ppm, TVOC: %d ppb", sensor_data->eco2, sensor_data->tvoc);

  sensor_data->state = k_ccs811_data_updated;
  return ESP_OK;
}

void ccs811_reset_on_error(ccs811_data_t *sensor_data)
{
  if (sensor_data->state & k_ccs811_error) {
    /* TODO: Reset error handler */
  }
}

void ccs811_tasks(void *sensor_data)
{
  ccs811_data_t *ccs811_data = (ccs811_data_t *)sensor_data;
  while (1) {
    if (ccs811_read(ccs811_data) == ESP_OK) {
      char *json = ccs811_data_to_json(ccs811_data);
      send_sensor_data_to_webserver(json);
      file_write_enqueue("ccs811.txt", json);
      free(json);
    } else {
      ccs811_reset_on_error(ccs811_data);
    }
    vTaskDelay(ccs811_polling_rate_ticks);
  }
}

