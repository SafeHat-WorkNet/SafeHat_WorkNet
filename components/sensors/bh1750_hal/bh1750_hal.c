/* components/sensors/bh1750_hal/bh1750_hal.c */

#include "bh1750_hal.h"
#include "file_write_manager.h"
#include "webserver_tasks.h"
#include "cJSON.h"
#include "common/i2c.h"
#include "esp_log.h"

/* Constants ******************************************************************/

const uint8_t    bh1750_i2c_address            = 0x23;
const i2c_port_t bh1750_i2c_bus                = I2C_NUM_0;
const char      *bh1750_tag                    = "BH1750";
const uint8_t    bh1750_scl_io                 = GPIO_NUM_22;
const uint8_t    bh1750_sda_io                 = GPIO_NUM_21;
const uint32_t   bh1750_i2c_freq_hz            = 100000;
const uint32_t   bh1750_polling_rate_ticks     = pdMS_TO_TICKS(5 * 1000);
const uint8_t    bh1750_max_retries            = 4;
const uint32_t   bh1750_initial_retry_interval = pdMS_TO_TICKS(15);
const uint32_t   bh1750_max_backoff_interval   = pdMS_TO_TICKS(8 * 60);

/* Public Functions ***********************************************************/

char *bh1750_data_to_json(const bh1750_data_t *data)
{
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    ESP_LOGE(bh1750_tag, "Failed to create JSON object.");
    return NULL;
  }

  if (!cJSON_AddStringToObject(json, "sensor_type", "light")) {
    ESP_LOGE(bh1750_tag, "Failed to add sensor_type to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "lux", data->lux)) {
    ESP_LOGE(bh1750_tag, "Failed to add lux to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  char *json_string = cJSON_PrintUnformatted(json);
  if (!json_string) {
    ESP_LOGE(bh1750_tag, "Failed to serialize JSON object.");
    cJSON_Delete(json);
    return NULL;
  }

  cJSON_Delete(json);
  return json_string;
}

esp_err_t bh1750_init(void *sensor_data)
{
  bh1750_data_t *bh1750_data = (bh1750_data_t *)sensor_data;
  ESP_LOGI(bh1750_tag, "Starting BH1750 Configuration");

  bh1750_data->i2c_address        = bh1750_i2c_address;
  bh1750_data->i2c_bus            = bh1750_i2c_bus;
  bh1750_data->lux                = -1.0;
  bh1750_data->state              = k_bh1750_uninitialized;
  bh1750_data->retry_count        = 0;
  bh1750_data->retry_interval     = bh1750_initial_retry_interval;
  bh1750_data->last_attempt_ticks = 0;

  /* Initialize the I2C bus */
  esp_err_t ret = priv_i2c_init(bh1750_scl_io, bh1750_sda_io, bh1750_i2c_freq_hz,
                                bh1750_i2c_bus, bh1750_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(bh1750_tag, "I2C driver install failed: %s", esp_err_to_name(ret));
    return ret;
  }

  /* Power on the sensor */
  ret = priv_i2c_write_byte(k_bh1750_power_on_cmd, bh1750_i2c_bus,
                            bh1750_i2c_address, bh1750_tag);
  if (ret != ESP_OK) {
    bh1750_data->state = k_bh1750_power_on_error;
    ESP_LOGE(bh1750_tag, "BH1750 Power On failed: %s", esp_err_to_name(ret));
    return ret;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  /* Reset the sensor */
  ret = priv_i2c_write_byte(k_bh1750_reset_cmd, bh1750_i2c_bus,
                            bh1750_i2c_address, bh1750_tag);
  if (ret != ESP_OK) {
    bh1750_data->state = k_bh1750_reset_error;
    ESP_LOGE(bh1750_tag, "BH1750 Reset failed: %s", esp_err_to_name(ret));
    return ret;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

    /* Set continuous measurement mode (low res) */
    ret = priv_i2c_write_byte(k_bh1750_cont_low_res_mode_cmd, bh1750_i2c_bus,
                              bh1750_i2c_address, bh1750_tag);

    if (ret != ESP_OK) {
      bh1750_data->state = k_bh1750_cont_low_res_error;
      ESP_LOGE(bh1750_tag, "BH1750 Set Mode failed: %s", esp_err_to_name(ret));
      return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));


  bh1750_data->state = k_bh1750_ready;
  ESP_LOGI(bh1750_tag, "BH1750 Configuration Complete");
  return ESP_OK;
}

esp_err_t bh1750_read(bh1750_data_t *sensor_data)
{
  uint8_t   data[2]; /* TODO: Move 2 to an enum or explain via a comment */
  esp_err_t ret = priv_i2c_read_bytes(data, 2, bh1750_i2c_bus, /* TODO: Move 2 to an enum or explain via a comment */
                                      bh1750_i2c_address, bh1750_tag);
  if (ret != ESP_OK) {
    sensor_data->lux   = -1.0;
    sensor_data->state = k_bh1750_error;
    ESP_LOGE(bh1750_tag, "Failed to read data from BH1750");
    return ESP_FAIL;
  }

  uint16_t raw_light_intensity = (data[0] << 8) | data[1]; /* TODO: Move 8 and 1 to either an enum or add better comment to explain */
  sensor_data->lux             = raw_light_intensity / 1.2; /* TODO: MOve 1.2 to a const or an enum */
  ESP_LOGI(bh1750_tag, "Measured light intensity: %f lux", sensor_data->lux);

  sensor_data->state = k_bh1750_data_updated;
  return ESP_OK;
}

void bh1750_reset_on_error(bh1750_data_t *sensor_data)
{
  if (sensor_data->state & k_bh1750_error) {
    TickType_t now_ticks = xTaskGetTickCount();
    if (now_ticks - sensor_data->last_attempt_ticks >= sensor_data->retry_interval) {
      sensor_data->last_attempt_ticks = now_ticks;

      if (bh1750_init(sensor_data) == ESP_OK) {
        sensor_data->state          = k_bh1750_ready;
        sensor_data->retry_count    = 0;
        sensor_data->retry_interval = bh1750_initial_retry_interval;
      } else {
        sensor_data->retry_count += 1;

        if (sensor_data->retry_count >= bh1750_max_retries) {
          sensor_data->retry_count    = 0;
          sensor_data->retry_interval = (sensor_data->retry_interval * 2 <= bh1750_max_backoff_interval) ?
                                        sensor_data->retry_interval * 2 :
                                        bh1750_max_backoff_interval;
        }
      }
    }
  }
}

void bh1750_tasks(void *sensor_data)
{
  bh1750_data_t *bh1750_data = (bh1750_data_t *)sensor_data;
  while (1) {
    if (bh1750_read(bh1750_data) == ESP_OK) {
      char *json = bh1750_data_to_json(bh1750_data);
      send_sensor_data_to_webserver(json);
      file_write_enqueue("bh1750.txt", json);
      free(json);
    } else {
      bh1750_reset_on_error(bh1750_data);
    }
    vTaskDelay(bh1750_polling_rate_ticks);
  }
}
