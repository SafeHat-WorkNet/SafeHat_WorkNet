/* components/sensors/mpu6050_hal/mpu6050_hal.c */

/* TODO: The values retrieved from this sensor seems a bit sus, needs to be configurea a bit better */

#include "mpu6050_hal.h"
#include "file_write_manager.h"
#include "webserver_tasks.h"
#include "cJSON.h"
#include "common/i2c.h"
#include "esp_log.h"
#include "driver/gpio.h"

/* Constants ******************************************************************/

const uint8_t    mpu6050_i2c_address        = 0x68;
const i2c_port_t mpu6050_i2c_bus            = I2C_NUM_0;
const char      *mpu6050_tag                = "MPU6050";
const uint8_t    mpu6050_scl_io             = GPIO_NUM_22;
const uint8_t    mpu6050_sda_io             = GPIO_NUM_21;
const uint32_t   mpu6050_i2c_freq_hz        = 100000;
const uint32_t   mpu6050_polling_rate_ticks = pdMS_TO_TICKS(5 * 1000);
const uint8_t    mpu6050_sample_rate_div    = 9;
const uint8_t    mpu6050_config_dlpf        = k_mpu6050_config_dlpf_44hz;
const uint8_t    mpu6050_int_io             = GPIO_NUM_26;

/**
 * @brief Static constant array of accelerometer configurations and scaling factors.
 *
 * The MPU6050 accelerometer has several sensitivity options that define the maximum
 * measurable acceleration range. Each configuration has a corresponding sensitivity
 * value, given in LSB/g, which allows conversion from raw sensor output to
 * acceleration in g.
 *
 * Benefits of configuring accelerometer sensitivity:
 * - Higher sensitivity (e.g., ±2g) offers finer resolution for small movements,
 *   ideal for low-speed applications or where precision is required.
 * - Lower sensitivity (e.g., ±16g) provides a wider measurement range, suitable
 *   for detecting high-impact or fast movements.
 *
 * Sensitivity options (in LSB/g) as per the MPU6050 datasheet:
 * - ±2g: 16384 LSB/g
 * - ±4g: 8192 LSB/g
 * - ±8g: 4096 LSB/g
 * - ±16g: 2048 LSB/g
 */
static const mpu6050_accel_config_t mpu6050_accel_configs[] = {
  { k_mpu6050_accel_fs_2g,  16384.0 }, /**< Sensitivity: 16384 LSB/g */
  { k_mpu6050_accel_fs_4g,  8192.0  }, /**< Sensitivity: 8192 LSB/g */
  { k_mpu6050_accel_fs_8g,  4096.0  }, /**< Sensitivity: 4096 LSB/g */
  { k_mpu6050_accel_fs_16g, 2048.0  }, /**< Sensitivity: 2048 LSB/g */
};

/**
 * @brief Static constant array of gyroscope configurations and scaling factors.
 *
 * The MPU6050 gyroscope provides several sensitivity options that define the maximum
 * measurable rotational speed range. Each configuration has an associated sensitivity
 * value in LSB/°/s, enabling conversion from raw sensor output to angular velocity
 * in degrees per second (°/s).
 *
 * Benefits of configuring gyroscope sensitivity:
 * - Higher sensitivity (e.g., ±250°/s) allows finer resolution for slow rotations,
 *   which is ideal for applications requiring precision.
 * - Lower sensitivity (e.g., ±2000°/s) enables a wider measurement range, useful
 *   for detecting fast or high-impact rotations.
 *
 * Sensitivity options (in LSB/°/s) as per the MPU6050 datasheet:
 * - ±250°/s: 131 LSB/°/s
 * - ±500°/s: 65.5 LSB/°/s
 * - ±1000°/s: 32.8 LSB/°/s
 * - ±2000°/s: 16.4 LSB/°/s
 */
static const mpu6050_gyro_config_t mpu6050_gyro_configs[] = {
  { k_mpu6050_gyro_fs_250dps,  131.0 }, /**< Sensitivity: 131 LSB/°/s */
  { k_mpu6050_gyro_fs_500dps,  65.5  }, /**< Sensitivity: 65.5 LSB/°/s */
  { k_mpu6050_gyro_fs_1000dps, 32.8  }, /**< Sensitivity: 32.8 LSB/°/s */
  { k_mpu6050_gyro_fs_2000dps, 16.4  }, /**< Sensitivity: 16.4 LSB/°/s */
};

static const uint8_t mpu6050_gyro_config_idx  = 3; /**< Index of chosen values from above (0: ±250°/s, 1: ±500°/s, etc.) */
static const uint8_t mpu6050_accel_config_idx = 3; /**< Index of chosen values from above (0: ±2g, 1: ±4g, etc.) */

/* Static (Private) Functions **************************************************/

/**
 * @brief Interrupt Service Routine (ISR) for handling MPU6050 data ready interrupts.
 *
 * This function is called when the MPU6050 asserts its INT pin, indicating that new data
 * is ready to be read. It gives the `data_ready_sem` semaphore to unblock the task waiting
 * to read the data.
 *
 * @param[in] arg Pointer to the `mpu6050_data_t` structure.
 *
 * @note This ISR should be kept as short as possible to avoid blocking other interrupts. 
 *       Ensure that the semaphore is properly initialized before enabling the interrupt.
 */
static void IRAM_ATTR priv_mpu6050_interrupt_handler(void *arg)
{
  mpu6050_data_t *sensor_data              = (mpu6050_data_t *)arg;
  BaseType_t      xHigherPriorityTaskWoken = pdFALSE;

  /* Give the semaphore to signal that data is ready */
  xSemaphoreGiveFromISR(sensor_data->data_ready_sem, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

/* Public Functions ***********************************************************/

char *mpu6050_data_to_json(const mpu6050_data_t *data)
{
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    ESP_LOGE(mpu6050_tag, "Failed to create JSON object.");
    return NULL;
  }

  if (!cJSON_AddStringToObject(json, "sensor_type", "accelerometer_gyroscope")) {
    ESP_LOGE(mpu6050_tag, "Failed to add sensor_type to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "accel_x", data->accel_x)) {
    ESP_LOGE(mpu6050_tag, "Failed to add accel_x to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "accel_y", data->accel_y)) {
    ESP_LOGE(mpu6050_tag, "Failed to add accel_y to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "accel_z", data->accel_z)) {
    ESP_LOGE(mpu6050_tag, "Failed to add accel_z to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "gyro_x", data->gyro_x)) {
    ESP_LOGE(mpu6050_tag, "Failed to add gyro_x to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "gyro_y", data->gyro_y)) {
    ESP_LOGE(mpu6050_tag, "Failed to add gyro_y to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "gyro_z", data->gyro_z)) {
    ESP_LOGE(mpu6050_tag, "Failed to add gyro_z to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  char *json_string = cJSON_PrintUnformatted(json);
  if (!json_string) {
    ESP_LOGE(mpu6050_tag, "Failed to serialize JSON object.");
    cJSON_Delete(json);
    return NULL;
  }

  cJSON_Delete(json);
  return json_string;
}

esp_err_t mpu6050_init(void *sensor_data)
{
  mpu6050_data_t *mpu6050_data = (mpu6050_data_t *)sensor_data;
  ESP_LOGI(mpu6050_tag, "Starting Configuration");

  mpu6050_data->i2c_address = mpu6050_i2c_address;
  mpu6050_data->i2c_bus     = mpu6050_i2c_bus;
  mpu6050_data->gyro_x      = mpu6050_data->gyro_y  = mpu6050_data->gyro_z  = 0.0f;
  mpu6050_data->accel_x     = mpu6050_data->accel_y = mpu6050_data->accel_z = 0.0f;
  mpu6050_data->state       = k_mpu6050_uninitialized; /* Start in uninitialized state */

  esp_err_t ret = priv_i2c_init(mpu6050_scl_io, mpu6050_sda_io,
                                mpu6050_i2c_freq_hz, mpu6050_i2c_bus, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "I2C driver install failed: %s", esp_err_to_name(ret));
    return ret;
  }

  /* Wake up the MPU6050 sensor */
  ret = priv_i2c_write_reg_byte(k_mpu6050_pwr_mgmt_1_cmd, k_mpu6050_power_on_cmd,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "MPU6050 power on failed");
    mpu6050_data->state = k_mpu6050_power_on_error;
    return ret;
  }

  /* Delay to allow the sensor to power on */
  vTaskDelay(pdMS_TO_TICKS(10));

  /* Reset the MPU6050 sensor */
  ret = priv_i2c_write_reg_byte(k_mpu6050_pwr_mgmt_1_cmd, k_mpu6050_reset_cmd,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "MPU6050 reset failed");
    mpu6050_data->state = k_mpu6050_reset_error;
    return ret;
  }

  /* Delay to allow the reset to take effect */
  vTaskDelay(pdMS_TO_TICKS(10));

  /* Wake up the sensor again after reset */
  ret = priv_i2c_write_reg_byte(k_mpu6050_pwr_mgmt_1_cmd, k_mpu6050_power_on_cmd,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "MPU6050 power on after reset failed");
    mpu6050_data->state = k_mpu6050_power_on_error;
    return ret;
  }

  /* Delay to allow the sensor to wake up */
  vTaskDelay(pdMS_TO_TICKS(10));

  /* Configure the sample rate divider */
  ret = priv_i2c_write_reg_byte(k_mpu6050_smplrt_div_cmd, mpu6050_sample_rate_div,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "MPU6050 sample rate configuration failed");
    return ret;
  }

  /* Configure the Digital Low Pass Filter (DLPF) */
  ret = priv_i2c_write_reg_byte(k_mpu6050_config_cmd, mpu6050_config_dlpf,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "MPU6050 DLPF configuration failed");
    return ret;
  }

  /* Configure the gyroscope full-scale range */
  ret = priv_i2c_write_reg_byte(k_mpu6050_gyro_config_cmd,
                                mpu6050_gyro_configs[mpu6050_gyro_config_idx].gyro_config,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "MPU6050 gyroscope configuration failed");
    return ret;
  }

  /* Configure the accelerometer full-scale range */
  ret = priv_i2c_write_reg_byte(k_mpu6050_accel_config_cmd,
                                mpu6050_accel_configs[mpu6050_accel_config_idx].accel_config,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "MPU6050 accelerometer configuration failed");
    return ret;
  }

  /* Verify the sensor by reading the WHO_AM_I register */
  uint8_t who_am_i = 0;
  ret              = priv_i2c_read_reg_bytes(k_mpu6050_who_am_i_cmd, &who_am_i, 
                                             1, mpu6050_i2c_bus, mpu6050_i2c_address, 
                                             mpu6050_tag);
  if (ret != ESP_OK || who_am_i != k_mpu6050_who_am_i_response) {
    ESP_LOGE(mpu6050_tag, "MPU6050 WHO_AM_I verification failed (read: 0x%02X)",
             who_am_i);
    return ret;
  }

  /* Create a binary semaphore for data readiness */
  mpu6050_data->data_ready_sem = xSemaphoreCreateBinary();
  if (mpu6050_data->data_ready_sem == NULL) {
    ESP_LOGE(mpu6050_tag, "Failed to create semaphore");
    return ESP_FAIL;
  }

  /* Configure the MPU6050 to generate Data Ready interrupts */
  ret = priv_i2c_write_reg_byte(k_mpu6050_int_enable_cmd, k_mpu6050_int_enable_data_rdy,
                                mpu6050_i2c_bus, mpu6050_i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "Failed to enable interrupts on MPU6050");
    return ret;
  }

  /* Configure the INT (interrupt) pin on the ESP32 */
  gpio_config_t io_conf = {};
  io_conf.intr_type     = GPIO_INTR_NEGEDGE; /* MPU6050 INT pin is active low */
  io_conf.pin_bit_mask  = (1ULL << mpu6050_int_io);
  io_conf.mode          = GPIO_MODE_INPUT;
  io_conf.pull_up_en    = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en  = GPIO_PULLDOWN_ENABLE;
  ret                   = gpio_config(&io_conf);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "GPIO configuration failed for INT pin");
    return ret;
  }

  /* Install GPIO ISR service */
  ret = gpio_install_isr_service(0);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(mpu6050_tag, "GPIO ISR service install failed");
    return ret;
  }

  /* Add ISR handler */
  ret = gpio_isr_handler_add(mpu6050_int_io, priv_mpu6050_interrupt_handler, (void*) mpu6050_data);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "Failed to add ISR handler for INT pin");
    return ret;
  }

  mpu6050_data->state = k_mpu6050_ready; /* Sensor is initialized */
  ESP_LOGI(mpu6050_tag, "Sensor Configuration Complete");
  return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_data_t *sensor_data)
{
  if (sensor_data == NULL) {
    ESP_LOGE(mpu6050_tag, "Sensor data pointer is NULL");
    return ESP_FAIL;
  }

  uint8_t accel_data[6];
  uint8_t gyro_data[6];

  /* Read accelerometer data starting from ACCEL_XOUT_H */
  esp_err_t ret = priv_i2c_read_reg_bytes(k_mpu6050_accel_xout_h_cmd, accel_data, 6,
                                          sensor_data->i2c_bus, sensor_data->i2c_address,
                                          mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "Failed to read accelerometer data from MPU6050");
    sensor_data->state = k_mpu6050_error;
    return ESP_FAIL;
  }

  /* Read gyroscope data starting from GYRO_XOUT_H */
  ret = priv_i2c_read_reg_bytes(k_mpu6050_gyro_xout_h_cmd, gyro_data, 6,
                                sensor_data->i2c_bus, 
                                sensor_data->i2c_address, mpu6050_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(mpu6050_tag, "Failed to read gyroscope data from MPU6050");
    sensor_data->state = k_mpu6050_error;
    return ESP_FAIL;
  }

  /* Combine high and low bytes to form the raw accelerometer data */
  int16_t accel_x_raw = (int16_t)((accel_data[0] << 8) | accel_data[1]);
  int16_t accel_y_raw = (int16_t)((accel_data[2] << 8) | accel_data[3]);
  int16_t accel_z_raw = (int16_t)((accel_data[4] << 8) | accel_data[5]);

  /* Combine high and low bytes to form the raw gyroscope data */
  int16_t gyro_x_raw = (int16_t)((gyro_data[0] << 8) | gyro_data[1]);
  int16_t gyro_y_raw = (int16_t)((gyro_data[2] << 8) | gyro_data[3]);
  int16_t gyro_z_raw = (int16_t)((gyro_data[4] << 8) | gyro_data[5]);

  /* Convert raw data to physical units by dividing by sensitivity */
  float accel_sensitivity = mpu6050_accel_configs[mpu6050_accel_config_idx].accel_scale;
  float gyro_sensitivity  = mpu6050_gyro_configs[mpu6050_gyro_config_idx].gyro_scale;

  sensor_data->accel_x = accel_x_raw / accel_sensitivity;
  sensor_data->accel_y = accel_y_raw / accel_sensitivity;
  sensor_data->accel_z = accel_z_raw / accel_sensitivity;

  sensor_data->gyro_x = gyro_x_raw / gyro_sensitivity;
  sensor_data->gyro_y = gyro_y_raw / gyro_sensitivity;
  sensor_data->gyro_z = gyro_z_raw / gyro_sensitivity;

  ESP_LOGI(mpu6050_tag, "Accel: [%f, %f, %f] g, Gyro: [%f, %f, %f] deg/s",
           sensor_data->accel_x, sensor_data->accel_y, sensor_data->accel_z,
           sensor_data->gyro_x, sensor_data->gyro_y, sensor_data->gyro_z);

  sensor_data->state = k_mpu6050_data_updated;
  return ESP_OK;
}

void mpu6050_reset_on_error(mpu6050_data_t *sensor_data)
{
  /* Check if the state indicates any error */
  if (sensor_data->state & k_mpu6050_error) {
    ESP_LOGI(mpu6050_tag, "Error detected. Attempting to reset the MPU6050 sensor.");

    /* Attempt to initialize/reset the sensor */
    if (mpu6050_init(sensor_data) == ESP_OK) {
      /* If successful, set the state to ready */
      sensor_data->state = k_mpu6050_ready;
      ESP_LOGI(mpu6050_tag, "MPU6050 sensor reset successfully. State is now ready.");
    } else {
      /* If reset fails, set the state to reset error */
      sensor_data->state = k_mpu6050_reset_error;
      ESP_LOGE(mpu6050_tag, "Failed to reset the MPU6050 sensor. State set to reset error.");
    }
  }
}

void mpu6050_tasks(void *sensor_data)
{
  mpu6050_data_t *mpu6050_data = (mpu6050_data_t *)sensor_data;
  while (1) {
    /* Wait indefinitely for the data_ready_sem semaphore */
    if (xSemaphoreTake(mpu6050_data->data_ready_sem, portMAX_DELAY) == pdTRUE) {
      if (mpu6050_read(mpu6050_data) == ESP_OK) {
        char *json = mpu6050_data_to_json(mpu6050_data);
        send_sensor_data_to_webserver(json);
        file_write_enqueue("mpu6050.txt", json);
        free(json);
      } else {
        mpu6050_reset_on_error(mpu6050_data);
      }
      vTaskDelay(mpu6050_polling_rate_ticks);
    }
  }
}
