/* components/storage/sd_card_hal/sd_card_hal.c */

/* TODO: CD & SDIO */
/* TODO: Error handler */

#include "sd_card_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "error_handler.h"

/* Constants ******************************************************************/

const char             *sd_card_tag                  = "SD_CARD";
const char             *sd_card_mount_path           = "/sdcard";
const uint8_t           sd_card_cs                   = GPIO_NUM_5;
const uint8_t           sd_card_data_to_card         = GPIO_NUM_23;
const uint8_t           sd_card_clk                  = GPIO_NUM_14;
const uint8_t           sd_card_data_from_card       = GPIO_NUM_19;
const uint32_t          sd_card_spi_freq_hz          = 1000000;     /* 1 MHz SPI frequency */
const spi_host_device_t sd_card_spi_host             = SPI2_HOST;
const uint8_t           sd_card_max_files            = 5;
const uint32_t          sd_card_allocation_unit_size = 16 * 1024;
const uint32_t          sd_card_max_transfer_sz      = 4092;        /* Default size in Bytes */
const uint8_t           sd_card_max_retries          = 5;
const uint32_t          sd_card_retry_delay_ms       = 500;         /* 500ms delay between retries */

/* Private Variables **********************************************************/

static sdmmc_card_t   *s_card                  = NULL; /**< Pointer to hold SD card descriptor */
static error_handler_t s_sd_card_error_handler = { 0 };

/* Private (Static) Functions *************************************************/

/**
 * @brief Internal function to unmount and cleanup resources.
 */
static void priv_sd_card_cleanup(void)
{
  if (s_card) {
    esp_vfs_fat_sdcard_unmount(sd_card_mount_path, s_card);
    s_card = NULL;
    ESP_LOGI(sd_card_tag, "SD card unmounted.");
  }
  spi_bus_free(sd_card_spi_host);
}

/* Public Functions ***********************************************************/

esp_err_t sd_card_init(void) 
{
  esp_err_t ret;
  uint8_t   retry_count = 0;

  ESP_LOGI(sd_card_tag, "Initializing SD card");

  while (retry_count < sd_card_max_retries) {
    /* Configure SPI host */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot         = sd_card_spi_host;           /* Use the defined SPI host */
    host.max_freq_khz = sd_card_spi_freq_hz / 1000; /* Convert Hz to kHz for the frequency */

    /* Configure SPI bus */
    spi_bus_config_t bus_cfg = {
      .mosi_io_num     = sd_card_data_to_card,
      .miso_io_num     = sd_card_data_from_card,
      .sclk_io_num     = sd_card_clk,
      .quadwp_io_num   = -1, /* Not used */
      .quadhd_io_num   = -1, /* Not used */
      .max_transfer_sz = sd_card_max_transfer_sz
    };

    ret = spi_bus_initialize(sd_card_spi_host, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
      ESP_LOGE(sd_card_tag, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
      vTaskDelay(pdMS_TO_TICKS(sd_card_retry_delay_ms));
      retry_count++;
      continue;
    }

    /* Configure SD card slot */
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = sd_card_cs;
    slot_config.host_id               = sd_card_spi_host;

    /* Filesystem mount configuration */
    esp_vfs_fat_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files              = sd_card_max_files,
      .allocation_unit_size   = sd_card_allocation_unit_size,
    };

    /* Mount the filesystem */
    ret = esp_vfs_fat_sdspi_mount(sd_card_mount_path, &host, &slot_config, &mount_config, &s_card);
    if (ret == ESP_OK) {
      /* Log SD card information */
      sdmmc_card_print_info(stdout, s_card);
      ESP_LOGI(sd_card_tag, "SD card initialized successfully");
      return ESP_OK;
    } else {
      ESP_LOGE(sd_card_tag, "Failed to mount filesystem: %s", esp_err_to_name(ret));
      priv_sd_card_cleanup();
      vTaskDelay(pdMS_TO_TICKS(sd_card_retry_delay_ms));
      retry_count++;
    }
  }

  ESP_LOGE(sd_card_tag, "SD card initialization failed after %d retries.", sd_card_max_retries);
  return ESP_FAIL;
}

