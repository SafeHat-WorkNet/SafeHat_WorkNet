/* components/camera/ov7670_hal/ov7670_hal.c */

/* 
 * "The OV7670 requires an external clock (12–24 MHz) on the XCLK pin.
 *  Without this, the module will not power up correctly."
 */

#include "ov7670_hal.h"
#include <inttypes.h>
#include "common/i2c.h"
#include "esp_log.h"
#include "freertos/task.h"

#ifdef USE_OV7670_XCLK_GPIO_27
#include "driver/ledc.h"
#endif

/* Contants *******************************************************************/

/* 
 * If we define USE_OV7670_XCLK_GPIO_27, we will configure LEDC on GPIO_NUM_27
 * to generate the XCLK. Otherwise, do nothing (external clock assumed).
 */
#ifdef USE_OV7670_XCLK_GPIO_27
static const uint32_t   ov7670_xclk_freq_hz = 24000000; /* 24 MHz */
static const gpio_num_t ov7670_xclk_gpio    = GPIO_NUM_27;
#endif

const char      *ov7670_tag                = "OV7670";
const uint8_t    ov7670_i2c_address        = (0x42 >> 1); /* 7-bit address */
const i2c_port_t ov7670_i2c_bus            = I2C_NUM_0;
const uint32_t   ov7670_i2c_freq_hz        = 100000;      /* 100 kHz */
const uint32_t   ov7670_polling_rate_ticks = pdMS_TO_TICKS(5000);
const uint8_t    ov7670_scl_io             = GPIO_NUM_22;
const uint8_t    ov7670_sda_io             = GPIO_NUM_21;

/* Private (Static) Functions *************************************************/

#ifdef USE_OV7670_XCLK_GPIO_27
/**
 * @brief Configure LEDC to generate XCLK on GPIO 27.
 * @param freq_hz The desired clock frequency (e.g., 24MHz).
 * @return ESP_OK on success, or error code on failure.
 */
static esp_err_t priv_configure_xclk_on_gpio_27(uint32_t freq_hz)
{
  ESP_LOGI(ov7670_tag, "Configuring XCLK on GPIO 27 at %" PRIu32 " Hz", freq_hz);

  /* LEDC Timer Configuration */
  ledc_timer_config_t ledc_timer = {
    .speed_mode      = LEDC_HIGH_SPEED_MODE,
    .timer_num       = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_1_BIT, /* 1-bit = 0% or 50% duty */
    .freq_hz         = freq_hz,
    .clk_cfg         = LEDC_AUTO_CLK
  };
  esp_err_t ret = ledc_timer_config(&ledc_timer);
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "Failed to configure LEDC timer (err=0x%x)", ret);
    return ret;
  }

  /* LEDC Channel Configuration */
  ledc_channel_config_t ledc_channel = {
    .gpio_num   = ov7670_xclk_gpio,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel    = LEDC_CHANNEL_0,
    .timer_sel  = LEDC_TIMER_0,
    .duty       = 1, /* minimal duty to produce a square wave */
    .hpoint     = 0
  };
  ret = ledc_channel_config(&ledc_channel);
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "Failed to configure LEDC channel (err=0x%x)", ret);
  }

  return ret;
}
#endif

/* Private Functions **********************************************************/

/**
 * @brief Applies the specified configuration settings to the OV7670 module.
 */
static esp_err_t priv_ov7670_apply_config(const ov7670_config_t *config)
{
  /* Combine resolution and format into COM7 register. */
  uint8_t   com7_value = (uint8_t)(config->resolution | config->output_format);
  esp_err_t ret        = priv_i2c_write_reg_byte(k_ov7670_reg_com7, com7_value,
                                                 ov7670_i2c_bus, ov7670_i2c_address,
                                                 ov7670_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "Failed to set COM7 (resolution/format).");
    return ret;
  }

  /* Set clock divider (CLKRC register). */
  ret = priv_i2c_write_reg_byte(k_ov7670_reg_clkrc, (uint8_t)config->clock_divider,
                                ov7670_i2c_bus, ov7670_i2c_address, ov7670_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "Failed to set CLKRC (clock divider).");
    return ret;
  }

  ESP_LOGI(ov7670_tag, "Configuration applied: COM7=0x%02X, CLKRC=0x%02X",
           com7_value, (uint8_t)config->clock_divider);
  return ESP_OK;
}

/**
 * @brief Applies default configuration settings to the OV7670 module.
 */
static esp_err_t priv_ov7670_configure_defaults(void)
{
  /* Example default settings: QVGA, RGB565, clock divider = 2. */
  ov7670_config_t default_config = {
    .resolution    = k_ov7670_res_qvga,
    .output_format = k_ov7670_output_rgb,
    .clock_divider = k_ov7670_clk_div_2,
  };
  return priv_ov7670_apply_config(&default_config);
}

/* Public Functions ***********************************************************/

esp_err_t ov7670_init(ov7670_data_t *camera_data)
{
  if (!camera_data) {
    ESP_LOGE(ov7670_tag, "Camera data pointer is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(ov7670_tag, "Initializing OV7670 Camera");

  /* 1. Initialize I2C interface */
  esp_err_t ret = priv_i2c_init(ov7670_scl_io, ov7670_sda_io,
                                ov7670_i2c_freq_hz, ov7670_i2c_bus,
                                ov7670_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "I2C initialization failed");
    camera_data->state = k_ov7670_config_error;
    return ret;
  }

#ifdef USE_OV7670_XCLK_GPIO_27
  /* 2. Configure the ESP32 to generate the XCLK on GPIO_NUM_27 */
  ret = priv_configure_xclk_on_gpio_27(ov7670_xclk_freq_hz);
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "Failed to configure XCLK on GPIO 27");
    camera_data->state = k_ov7670_config_error;
    return ret;
  }
  ESP_LOGI(ov7670_tag, "XCLK is now driven on GPIO 27 at %" PRIu32 " Hz",
           ov7670_xclk_freq_hz);
#else
  /* If not defined, do nothing: we assume an external clock is provided */
  ESP_LOGI(ov7670_tag, "No internal XCLK; an external clock is expected.");
#endif

  /* 3. Apply default settings */
  ret = priv_ov7670_configure_defaults();
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "Default configuration failed");
    camera_data->state = k_ov7670_config_error;
    return ret;
  }

  camera_data->state = k_ov7670_ready;
  ESP_LOGI(ov7670_tag, "OV7670 initialization complete (state=%d)", camera_data->state);
  return ESP_OK;
}

esp_err_t ov7670_configure(ov7670_data_t *camera_data)
{
  if (!camera_data) {
    ESP_LOGE(ov7670_tag, "Camera data pointer is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(ov7670_tag, "Applying new configuration to OV7670 Camera");
  esp_err_t ret = priv_ov7670_apply_config(&camera_data->config);
  if (ret != ESP_OK) {
    ESP_LOGE(ov7670_tag, "Configuration failed");
    camera_data->state = k_ov7670_config_error;
    return ret;
  }

  ESP_LOGI(ov7670_tag, "Configuration successfully applied (state=%d)",
           camera_data->state);
  return ESP_OK;
}

void ov7670_reset_on_error(ov7670_data_t *camera_data)
{
  if (!camera_data) {
    ESP_LOGE(ov7670_tag, "Camera data pointer is NULL");
    return;
  }

  if (camera_data->state == k_ov7670_config_error) {
    TickType_t current_ticks = xTaskGetTickCount();

    /* Check if enough time has elapsed since last attempt */
    if ((current_ticks - camera_data->last_attempt_ticks) >= camera_data->retry_interval) {
      ESP_LOGW(ov7670_tag, "Attempting to reset OV7670 (retries=%d)", camera_data->retries);

      /* Attempt re-initialization */
      if (ov7670_init(camera_data) == ESP_OK) {
        camera_data->retries        = 0;
        camera_data->retry_interval = pdMS_TO_TICKS(15000);
        ESP_LOGI(ov7670_tag, "OV7670 re-initialized successfully");
      } else {
        camera_data->retries++;
        camera_data->retry_interval = pdMS_TO_TICKS(camera_data->retry_interval * 2);
        ESP_LOGE(ov7670_tag, "OV7670 re-initialization failed. Next retry interval = %" PRIu32 " ms",
                 (camera_data->retry_interval * portTICK_PERIOD_MS));
      }
      camera_data->last_attempt_ticks = current_ticks;
    }
  }
}

void ov7670_tasks(void *camera_data_)
{
  ov7670_data_t *camera_data = (ov7670_data_t *)camera_data_;
  if (!camera_data) {
    ESP_LOGE(ov7670_tag, "Camera data pointer is NULL in ov7670_tasks");
    vTaskDelete(NULL);
    return;
  }

  while (1) {
    if (camera_data->state == k_ov7670_config_error) {
      ov7670_reset_on_error(camera_data);
    }
    vTaskDelay(ov7670_polling_rate_ticks);
  }
}

