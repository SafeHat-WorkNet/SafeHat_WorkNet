/* components/common/error_handler.c */

#include "error_handler.h"

/* Public Functions ***********************************************************/

void error_handler_init(error_handler_t *handler,
                        const char *tag,
                        uint8_t allowed_fail_attempts,
                        uint8_t max_retries,
                        uint32_t initial_retry_interval,
                        uint32_t max_backoff_interval) 
{
  handler->retry_count            = 0;
  handler->retry_interval         = initial_retry_interval;
  handler->last_attempt_ticks     = 0;
  handler->fail_count             = 0;
  handler->allowed_fail_attempts  = allowed_fail_attempts;
  handler->max_retries            = max_retries;
  handler->initial_retry_interval = initial_retry_interval;
  handler->max_backoff_interval   = max_backoff_interval;
  handler->tag                    = tag;
}

esp_err_t error_handler_reset(error_handler_t *handler,
                              uint8_t current_fail_count,
                              esp_err_t (*init_func)(void*),
                              void *init_data) 
{
  if (current_fail_count >= handler->allowed_fail_attempts) {
    TickType_t current_ticks = xTaskGetTickCount();
    
    if ((current_ticks - handler->last_attempt_ticks) > handler->retry_interval) {
      ESP_LOGI(handler->tag, "Attempting to reset component");
      
      esp_err_t ret = init_func(init_data);
      if (ret == ESP_OK) {
        handler->retry_count    = 0;
        handler->retry_interval = handler->initial_retry_interval;
        handler->fail_count     = 0;
        ESP_LOGI(handler->tag, "Component reset successfully");
        return ESP_OK;
      } else {
        handler->retry_count++;
        if (handler->retry_count >= handler->max_retries) {
          handler->retry_count    = 0;
          handler->retry_interval = (handler->retry_interval * 2 > handler->max_backoff_interval) ?
                                    handler->max_backoff_interval :
                                    handler->retry_interval * 2;
        }
        ESP_LOGE(handler->tag, "Component reset failed, retry count: %d, next interval: %lu",
                 handler->retry_count, handler->retry_interval);
      }
      
      handler->last_attempt_ticks = current_ticks;
      return ESP_FAIL;
    }
  }
  return ESP_FAIL;
} 