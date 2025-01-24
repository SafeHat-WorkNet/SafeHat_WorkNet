/* components/storage/sd_card_hal/include/sd_card_hal.h */

#ifndef MESHNET_SD_CARD_HAL_H
#define MESHNET_SD_CARD_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "driver/spi_common.h"

/* Constants ******************************************************************/

/* NOTE: The terms 'data_to_card' (Data to Card) and 'data_from_card' (Data from Card) are used 
 * to promote inclusively and align with modern engineering practices, ensuring that technical
 * language remains respectful and accessible to all developers.
 */
extern const char             *sd_card_tag;                  /**< Tag for Logging */
extern const char             *sd_card_mount_path;           /**< Root path for the mounting */
extern const uint8_t           sd_card_cs;                   /**< GPIO pin for SPI Chip Select (CS) */
extern const uint8_t           sd_card_data_to_card;         /**< GPIO pin for SPI communication into the SD card (DI) */
extern const uint8_t           sd_card_data_from_card;       /**< GPIO pin for SPI communication out of the SD card (DO) */
extern const uint8_t           sd_card_clk;                  /**< GPIO pin for SPI clock (SCLK / CLK) */
extern const uint8_t           sd_card_clk;                  /**< GPIO pin for SPI clock (SCLK / CLK) */
extern const uint32_t          sd_card_spi_freq_hz;          /**< SPI bus frequency in Hz for SD card */
extern const spi_host_device_t sd_card_spi_host;             /**< Default SPI host for SD card interface */
extern const uint8_t           sd_card_max_files;            /**< Maximum number of files that can be opened simultaneously */
extern const uint32_t          sd_card_allocation_unit_size; /**< Alllocation unit size for SD card filesystem in Bytes */
extern const uint32_t          sd_card_max_transfer_sz;      /**< Maximum transfer size for SPI transactions in Bytes */
extern const uint8_t           sd_card_max_retries;          /**< Maximum retries after init fails */
extern const uint32_t          sd_card_retry_delay_ms;       /**< The delay between retries */

/* Public Functions ***********************************************************/

/**
 * @brief Initializes the SD card for file operations.
 *
 * Mounts the SD card filesystem using FATFS. The card must be properly formatted
 * with a FAT filesystem for successful mounting. Uses the SPI host configured in
 * `sd_card_spi_host`.
 *
 * @return
 * - `ESP_OK`         if the initialization is successful.
 * - `ESP_ERR_NO_MEM` if there is insufficient memory.
 * - `ESP_FAIL`       if mounting fails.
 */
esp_err_t sd_card_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_SD_CARD_HAL_H */

