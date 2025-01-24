/* components/common/include/error_handler.h */

#ifndef MESHNET_ERROR_HANDLER_H
#define MESHNET_ERROR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/**
 * @brief Error handler structure
 *
 * @param retry_count            Number of consecutive reinitialization attempts
 * @param retry_interval         Current interval between reinitialization attempts, in ticks
 * @param last_attempt_ticks     Tick count of the last reinitialization attempt
 * @param fail_count             Number of consecutive failures before triggering reset
 * @param allowed_fail_attempts  Maximum number of failures allowed before reset
 * @param max_retries            Maximum number of reset retries before increasing interval
 * @param initial_retry_interval Initial retry interval in ticks
 * @param max_backoff_interval   Maximum backoff interval in ticks
 * @param tag                    Logging tag for the component
 */
typedef struct {
  uint8_t     retry_count;            /**< Number of consecutive reinitialization attempts */
  uint32_t    retry_interval;         /**< Current interval between reinitialization attempts, in ticks */
  TickType_t  last_attempt_ticks;     /**< Tick count of the last reinitialization attempt */
  uint8_t     fail_count;             /**< Number of consecutive failures before triggering reset */
  uint8_t     allowed_fail_attempts;  /**< Maximum number of failures allowed before reset */
  uint8_t     max_retries;            /**< Maximum number of reset retries before increasing interval */
  uint32_t    initial_retry_interval; /**< Initial retry interval in ticks */
  uint32_t    max_backoff_interval;   /**< Maximum backoff interval in ticks */
  const char *tag;                    /**< Logging tag for the component */
} error_handler_t;

/**
 * @brief Initialize the error handler structure with default values
 *
 * @param handler Pointer to error_handler_t structure
 * @param tag Logging tag for the component
 * @param allowed_fail_attempts Number of failures allowed before reset
 * @param max_retries Maximum number of reset retries
 * @param initial_retry_interval Initial retry interval in ticks
 * @param max_backoff_interval Maximum backoff interval in ticks
 */
void error_handler_init(error_handler_t *handler,
                        const char *tag,
                        uint8_t allowed_fail_attempts,
                        uint8_t max_retries,
                        uint32_t initial_retry_interval,
                        uint32_t max_backoff_interval);

/**
 * @brief Handle error recovery with exponential backoff
 *
 * @param handler Pointer to error_handler_t structure
 * @param current_fail_count Current number of consecutive failures
 * @param init_func Function pointer to component initialization function
 * @param init_data Pointer to component data structure
 * @return esp_err_t ESP_OK if reset successful, ESP_FAIL otherwise
 */
esp_err_t error_handler_reset(error_handler_t *handler,
                            uint8_t current_fail_count,
                            esp_err_t (*init_func)(void*),
                            void *init_data);

#ifdef __cplusplus
}
#endif

#endif /* MESHNET_ERROR_HANDLER_H */