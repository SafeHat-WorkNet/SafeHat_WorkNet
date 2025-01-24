/* components/common/include/common/i2c.h */

#ifndef MESHNET_I2C_H
#define MESHNET_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"

/* Constants ******************************************************************/

extern const uint32_t i2c_timeout_ticks; /**< Timeout for I2C commands in ticks */

/* Private Functions **********************************************************/

/**
 * @brief Initializes the I2C interface with specified parameters.
 *
 * Configures the I2C interface with given SCL and SDA pins, frequency, and bus number.
 * Initializes the driver in controller mode without allocating buffers.
 *
 * @param[in] scl_io  I2C clock line (SCL) pin number.
 * @param[in] sda_io  I2C data line (SDA) pin number.
 * @param[in] freq_hz Communication frequency in Hertz.
 * @param[in] i2c_bus I2C bus number to use.
 * @param[in] tag     Logging tag for error messages.
 *
 * @return
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` error codes on failure.
 *
 * @note 
 * - Internal pull-ups are enabled for SDA and SCL.
 */
esp_err_t priv_i2c_init(uint8_t scl_io, uint8_t sda_io, uint32_t freq_hz,
                        i2c_port_t i2c_bus, const char *tag);

/**
 * @brief Writes a single byte to a specific I2C device.
 *
 * Sends a byte of data to the specified device using the provided bus and address.
 *
 * @param[in] data        Byte of data to send.
 * @param[in] i2c_bus     I2C bus number.
 * @param[in] i2c_address 7-bit I2C address of the target device.
 * @param[in] tag         Logging tag for error messages.
 *
 * @return
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` error codes on failure.
 *
 * @note 
 * - No semaphore checks or concurrency protection is implemented.
 */
esp_err_t priv_i2c_write_byte(uint8_t data, i2c_port_t i2c_bus,
                              uint8_t i2c_address, const char *tag);

/**
 * @brief Reads multiple bytes from a specific I2C device.
 *
 * Reads data from a target device using the specified bus and address.
 *
 * @param[out] data       Buffer to store the read data.
 * @param[in] len         Number of bytes to read.
 * @param[in] i2c_bus     I2C bus number.
 * @param[in] i2c_address 7-bit I2C address of the target device.
 * @param[in] tag         Logging tag for error messages.
 *
 * @return
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` error codes on failure.
 *
 * @note 
 * - This function does not include semaphore checks.
 */
esp_err_t priv_i2c_read_bytes(uint8_t *data, size_t len, i2c_port_t i2c_bus,
                              uint8_t i2c_address, const char *tag);

/**
 * @brief Writes a byte to a specific register on an I2C device.
 *
 * Writes data to a designated register using the provided bus and device address.
 *
 * @param[in] reg_addr    Register address to write to.
 * @param[in] data        Data byte to write.
 * @param[in] i2c_bus     I2C bus number.
 * @param[in] i2c_address 7-bit I2C address of the target device.
 * @param[in] tag         Logging tag for error messages.
 *
 * @return
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` error codes on failure.
 *
 * @note 
 * - No concurrency protection is implemented.
 */
esp_err_t priv_i2c_write_reg_byte(uint8_t reg_addr, uint8_t data,
                                  i2c_port_t i2c_bus, uint8_t i2c_address,
                                  const char *tag);

/**
 * @brief Reads multiple bytes starting from a specific register on an I2C device.
 *
 * Reads data from consecutive registers of a target device.
 *
 * @param[in] reg_addr    Starting register address to read from.
 * @param[out] data       Buffer to store the read data.
 * @param[in] len         Number of bytes to read.
 * @param[in] i2c_bus     I2C bus number.
 * @param[in] i2c_address 7-bit I2C address of the target device.
 * @param[in] tag         Logging tag for error messages.
 *
 * @return
 * - `ESP_OK` on success.
 * - Relevant `esp_err_t` error codes on failure.
 *
 * @note 
 * - No semaphore checks or concurrency protection is implemented.
 */
esp_err_t priv_i2c_read_reg_bytes(uint8_t reg_addr, uint8_t *data, size_t len,
                                  i2c_port_t i2c_bus, uint8_t i2c_address,
                                  const char *tag);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_I2C_H */

