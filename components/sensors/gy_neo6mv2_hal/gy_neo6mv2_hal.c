/* components/sensors/gy_neo6mv2_hal/gy_neo6mv2_hal.c */

/* TODO: UBLOCK's app works, but this doesn't :( sorrow */

#include "gy_neo6mv2_hal.h"
#include <string.h>
#include <stdlib.h>
#include "esp_err.h"
#include "file_write_manager.h"
#include "webserver_tasks.h"
#include "cJSON.h"
#include "common/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* Constants *******************************************************************/

const char       *gy_neo6mv2_tag                    = "GY-NEO6MV2";
const uint8_t     gy_neo6mv2_tx_io                  = GPIO_NUM_17;
const uint8_t     gy_neo6mv2_rx_io                  = GPIO_NUM_16;
const uart_port_t gy_neo6mv2_uart_num               = UART_NUM_2;
const uint32_t    gy_neo6mv2_uart_baudrate          = 9600;
const uint32_t    gy_neo6mv2_polling_rate_ticks     = pdMS_TO_TICKS(5 * 100);
const uint8_t     gy_neo6mv2_max_retries            = 4;
const uint32_t    gy_neo6mv2_initial_retry_interval = pdMS_TO_TICKS(15 * 1000);
const uint32_t    gy_neo6mv2_max_backoff_interval   = pdMS_TO_TICKS(480 * 1000);

/* Globals (Static) ***********************************************************/

static char        s_gy_neo6mv2_sentence_buffer[gy_neo6mv2_sentence_buffer_size]; /**< Buffer for assembling fragmented NMEA sentences received from the GPS module. */
static uint32_t    s_gy_neo6mv2_sentence_index = 0;                               /**< Index tracking the current position in the sentence buffer. */
static satellite_t s_gy_neo6mv2_satellites[gy_neo6mv2_max_satellites];            /**< Buffer to store parsed satellite information from GPGSV sentences. */
static uint8_t     s_gy_neo6mv2_satellite_count = 0;                              /**< Counter for the number of satellites currently stored in the buffer. */

/* Static (Private) Functions *************************************************/

/**
 * @brief Converts a GPS coordinate from NMEA format to decimal degrees.
 *
 * Parses a coordinate in the NMEA format (DDMM.MMMM) used by the GY-NEO6MV2 GPS module 
 * and converts it to decimal degrees. Adjusts the result based on the specified hemisphere.
 *
 * @param[in] coord_str  Pointer to a string containing the coordinate in NMEA format.
 * @param[in] hemisphere Pointer to a character indicating the hemisphere ('N', 'S', 'E', 'W').
 *
 * @return The coordinate in decimal degrees.
 */
static float priv_gy_neo6mv2_parse_coordinate(const char *coord_str, const char *hemisphere)
{
  float    coord           = atof(coord_str);
  uint32_t degrees         = (uint32_t)(coord / 100);
  float    minutes         = coord - (degrees * 100);
  float    decimal_degrees = degrees + (minutes / 60.0);

  if (hemisphere[0] == 'S' || hemisphere[0] == 'W') {
    decimal_degrees = -decimal_degrees;
  }

  return decimal_degrees;
}

/**
 * @brief Validates the checksum of an NMEA sentence.
 *
 * Computes the checksum for an NMEA sentence and compares it with the checksum 
 * provided in the sentence to verify its validity.
 *
 * @param[in] sentence Pointer to a null-terminated NMEA sentence string.
 *
 * @return 
 * - `true`  if the checksum is valid.
 * - `false` otherwise.
 */
static bool priv_gy_neo6mv2_validate_nmea_checksum(const char *sentence)
{
  if (!sentence || sentence[0] != '$') {
    return false;
  }

  const char *checksum_start = strchr(sentence, '*');
  if (!checksum_start || (checksum_start - sentence) > strlen(sentence)) {
    return false;
  }

  uint8_t calculated_checksum = 0;
  for (const char *p = sentence + 1; p < checksum_start; p++) {
    calculated_checksum ^= *p;
  }

  uint8_t sent_checksum = (uint8_t)strtol(checksum_start + 1, NULL, 16);
  return calculated_checksum == sent_checksum;
}

/**
 * @brief Splits an NMEA sentence into individual fields.
 *
 * Tokenizes an NMEA sentence using commas as delimiters and stores the extracted 
 * fields in the provided array. Unused fields are set to `NULL`.
 *
 * @param[in,out] sentence   Pointer to the NMEA sentence (modified in place).
 * @param[out]    fields     Array of pointers to store the extracted fields.
 * @param[in]     max_fields Maximum number of fields to extract.
 */
static void priv_gy_neo6mv2_split_nmea_sentence(char *sentence, char **fields, size_t max_fields)
{
  if (!sentence || !fields) {
    return;
  }

  size_t index = 0;
  char  *token = strtok(sentence, ",");
  while (token != NULL && index < max_fields) {
    fields[index++] = token;
    token           = strtok(NULL, ",");
  }

  for (; index < max_fields; index++) {
    fields[index] = NULL;
  }
}

/**
 * @brief Adds satellite data to the buffer.
 *
 * Stores a satellite's information in the buffer if space is available. Logs a 
 * warning and discards the data if the buffer is full.
 *
 * @param[in] prn       Satellite ID (PRN).
 * @param[in] elevation Satellite elevation in degrees.
 * @param[in] azimuth   Satellite azimuth in degrees.
 * @param[in] snr       Signal-to-Noise Ratio (SNR).
 */
static void priv_gy_neo6mv2_add_satellite(uint8_t prn, uint8_t elevation, uint16_t azimuth, 
                                          uint8_t snr)
{
  if (s_gy_neo6mv2_satellite_count < gy_neo6mv2_max_satellites) {
    satellite_t *sat = &(s_gy_neo6mv2_satellites[s_gy_neo6mv2_satellite_count]);
    sat->prn         = prn;
    sat->elevation   = elevation;
    sat->azimuth     = azimuth;
    sat->snr         = snr;
    s_gy_neo6mv2_satellite_count++;

    ESP_LOGI(gy_neo6mv2_tag, "Satellite added: PRN=%d, Elevation=%d, Azimuth=%d, SNR=%d",
             prn, elevation, azimuth, snr);
  } else {
    ESP_LOGW(gy_neo6mv2_tag, "Satellite buffer full, cannot add PRN=%d", prn);
  }
}

/**
 * @brief Clears all data from the satellite buffer.
 *
 * Resets the satellite buffer by removing all stored data and setting the 
 * satellite count to zero.
 */
static void priv_gy_neo6mv2_clear_satellites(void)
{
  s_gy_neo6mv2_satellite_count = 0;
  memset(s_gy_neo6mv2_satellites, 0, sizeof(s_gy_neo6mv2_satellites));
  ESP_LOGI(gy_neo6mv2_tag, "Satellite buffer cleared.");
}

/**
 * @brief Retrieves satellite data from the buffer.
 *
 * Copies satellite data from the internal buffer into the provided output buffer 
 * for external processing.
 *
 * @param[out] satellites Pointer to the buffer where satellite data will be stored.
 * @param[in]  max_count  Maximum number of satellites to copy.
 *
 * @return The number of satellites copied into the output buffer.
 */
static uint8_t priv_gy_neo6mv2_get_satellites(satellite_t *satellites, uint8_t max_count)
{
  uint8_t count = (s_gy_neo6mv2_satellite_count < max_count) ?
                   s_gy_neo6mv2_satellite_count : max_count;

  memcpy(satellites, s_gy_neo6mv2_satellites, count * sizeof(satellite_t));
  ESP_LOGI(gy_neo6mv2_tag, "Retrieved %d satellites from the buffer.", count);
  return count;
}

/* Public Functions ***********************************************************/

char *gy_neo6mv2_data_to_json(const gy_neo6mv2_data_t *gy_neo6mv2_data)
{
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to create JSON object.");
    return NULL;
  }

  if (!cJSON_AddStringToObject(json, "sensor_type", "gps")) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add sensor_type to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "latitude", gy_neo6mv2_data->latitude)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add latitude to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "longitude", gy_neo6mv2_data->longitude)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add longitude to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "speed", gy_neo6mv2_data->speed)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add speed to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddStringToObject(json, "time", gy_neo6mv2_data->time)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add time to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "fix_status", gy_neo6mv2_data->fix_status)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add fix_status to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "satellite_count", gy_neo6mv2_data->satellite_count)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add satellite_count to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "hdop", gy_neo6mv2_data->hdop)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add hdop to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "retry_count", gy_neo6mv2_data->retry_count)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add retry_count to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  if (!cJSON_AddNumberToObject(json, "retry_interval", gy_neo6mv2_data->retry_interval)) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to add retry_interval to JSON.");
    cJSON_Delete(json);
    return NULL;
  }

  char *json_string = cJSON_PrintUnformatted(json);
  if (!json_string) {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to serialize JSON object.");
    cJSON_Delete(json);
    return NULL;
  }

  cJSON_Delete(json);
  return json_string;
}

esp_err_t gy_neo6mv2_init(void *sensor_data)
{
  ESP_LOGI(gy_neo6mv2_tag, "Initializing GPS module");

  /* Initialize UART using the common UART function */
  esp_err_t ret = priv_uart_init(gy_neo6mv2_tx_io, gy_neo6mv2_rx_io, gy_neo6mv2_uart_baudrate,
                                 gy_neo6mv2_uart_num, gy_neo6mv2_tag);
  if (ret != ESP_OK) {
    ESP_LOGE(gy_neo6mv2_tag, "UART initialization failed");
    return ret;
  }

  /* Allow time for the GPS module to warm up */
  vTaskDelay(pdMS_TO_TICKS(5000));

  /* Initialize GPS gy_neo6mv2_data fields */
  gy_neo6mv2_data_t *gy_neo6mv2_data  = (gy_neo6mv2_data_t *)sensor_data;
  gy_neo6mv2_data->latitude           = 0.0;                               /* Default latitude */
  gy_neo6mv2_data->longitude          = 0.0;                               /* Default longitude */
  gy_neo6mv2_data->speed              = 0.0;                               /* Default speed */
  gy_neo6mv2_data->fix_status         = 0;                                 /* No fix initially */
  gy_neo6mv2_data->satellite_count    = 0;                                 /* No satellites initially */
  gy_neo6mv2_data->hdop               = 99.99;                             /* Default HDOP value */
  gy_neo6mv2_data->state              = k_gy_neo6mv2_uninitialized;        /* Initial state */
  gy_neo6mv2_data->retry_count        = 0;                                 /* Reset retry count */
  gy_neo6mv2_data->retry_interval     = gy_neo6mv2_initial_retry_interval; /* Default retry interval */
  gy_neo6mv2_data->last_attempt_ticks = 0;                                 /* Reset last attempt ticks */
  memset(gy_neo6mv2_data->time, 0, sizeof(gy_neo6mv2_data->time));         /* Clear time field */

  ESP_LOGI(gy_neo6mv2_tag, "GPS module initialized successfully");
  return ESP_OK;
}

esp_err_t gy_neo6mv2_read(gy_neo6mv2_data_t *sensor_data)
{
  uint8_t uart_rx_buffer[gy_neo6mv2_sentence_buffer_size];
  int32_t length = 0;

  /* Read from UART */
  esp_err_t ret = priv_uart_read(uart_rx_buffer, sizeof(uart_rx_buffer),
                                 &length, gy_neo6mv2_uart_num, gy_neo6mv2_tag);

  if (ret == ESP_OK && length > 0) {
    for (uint32_t i = 0; i < length; i++) {
      char c = uart_rx_buffer[i];

      /* Accumulate characters in the sentence buffer */
      if (s_gy_neo6mv2_sentence_index < sizeof(s_gy_neo6mv2_sentence_buffer) - 1) {
        s_gy_neo6mv2_sentence_buffer[s_gy_neo6mv2_sentence_index++] = c;
      }

      /* Process a complete sentence (ends with \n) */
      if (c == '\n') {
        s_gy_neo6mv2_sentence_buffer[s_gy_neo6mv2_sentence_index] = '\0'; /* Null-terminate */

        /* Strip trailing \r and \n characters */
        while (s_gy_neo6mv2_sentence_index > 0 &&
            (s_gy_neo6mv2_sentence_buffer[s_gy_neo6mv2_sentence_index - 1] == '\r' ||
             s_gy_neo6mv2_sentence_buffer[s_gy_neo6mv2_sentence_index - 1] == '\n')) {
          s_gy_neo6mv2_sentence_buffer[--s_gy_neo6mv2_sentence_index] = '\0';
        }

        /* Log raw sentence */
        ESP_LOGI(gy_neo6mv2_tag, "Raw NMEA sentence: %s", s_gy_neo6mv2_sentence_buffer);

        /* Validate checksum */
        if (!priv_gy_neo6mv2_validate_nmea_checksum(s_gy_neo6mv2_sentence_buffer)) {
          ESP_LOGW(gy_neo6mv2_tag, "Invalid NMEA checksum: %s", s_gy_neo6mv2_sentence_buffer);
          s_gy_neo6mv2_sentence_index = 0;
          continue;
        }

        /* Parse specific sentences */
        if (strstr(s_gy_neo6mv2_sentence_buffer, "$GPRMC") == s_gy_neo6mv2_sentence_buffer) {
          /* Parse GPRMC sentence */
          char *fields[12] = { 0 };
          priv_gy_neo6mv2_split_nmea_sentence(s_gy_neo6mv2_sentence_buffer, fields, 12);

          /* Extract and log status */
          if (fields[2]) {
            const char *status = fields[2];
            ESP_LOGI(gy_neo6mv2_tag, "GPRMC Status: %s (%s)",
                     status, (status[0] == 'A') ? "Fix acquired" : "No fix");

            /* Process only valid readings */
            if (status[0] == 'A') {
              /* Extract latitude, longitude, and other data */
              sensor_data->latitude  = priv_gy_neo6mv2_parse_coordinate(fields[3], fields[4]);
              sensor_data->longitude = priv_gy_neo6mv2_parse_coordinate(fields[5], fields[6]);
              sensor_data->speed     = fields[7] ? atof(fields[7]) : 0.0;

              /* Update valid fix status */
              sensor_data->fix_status = 1; /* Fix acquired */
              strncpy(sensor_data->time, fields[1], sizeof(sensor_data->time) - 1);

              ESP_LOGI(gy_neo6mv2_tag, "Valid fix: Lat=%f, Lon=%f, Speed=%f",
                       sensor_data->latitude, sensor_data->longitude, 
                       sensor_data->speed);
            } else {
              sensor_data->fix_status = 0; /* No fix */
              ESP_LOGW(gy_neo6mv2_tag, "Skipping invalid GPS reading.");
            }
          }
        } else if (strstr(s_gy_neo6mv2_sentence_buffer, "$GPGSV") == s_gy_neo6mv2_sentence_buffer) {
          /* Parse GPGSV sentence for satellite information */
          char *fields[20] = { 0 }; /* TODO: Move 20 to an enum or const */
          priv_gy_neo6mv2_split_nmea_sentence(s_gy_neo6mv2_sentence_buffer, fields, 20); /* TODO: Move 20 to an enum or const */

          uint8_t total_sentences  = fields[1] ? (uint8_t)atoi(fields[1]) : 0;
          uint8_t sentence_number  = fields[2] ? (uint8_t)atoi(fields[2]) : 0;
          uint8_t total_satellites = fields[3] ? (uint8_t)atoi(fields[3]) : 0;

          ESP_LOGI(gy_neo6mv2_tag, "GPGSV: Sentence %u of %u, Total Satellites in view: %u",
                   sentence_number, total_sentences, total_satellites);

          /* Clear satellite data if this is the first sentence */
          if (sentence_number == 1) {
            priv_gy_neo6mv2_clear_satellites();
          }

          /* Parse satellite details (up to 4 satellites per GPGSV sentence) */
          for (uint8_t i = 4; i < 20; i += 4) { /* TODO: Move 4 and 20 either into enum / const / add a comment */
            if (fields[i] && fields[i + 1] && fields[i + 2] && fields[i + 3]) {
              uint8_t prn       = (uint8_t)atoi(fields[i]);      /* Satellite ID (PRN) */
              uint8_t elevation = (uint8_t)atoi(fields[i + 1]);  /* Elevation in degrees */
              uint16_t azimuth  = (uint16_t)atoi(fields[i + 2]); /* Azimuth in degrees */
              uint8_t snr       = (uint8_t)atoi(fields[i + 3]);  /* SNR (Signal-to-Noise Ratio) */

              /* Store satellite data */
              priv_gy_neo6mv2_add_satellite(prn, elevation, azimuth, snr);
            }
          }
        }

        /* Reset buffer for next sentence */
        s_gy_neo6mv2_sentence_index = 0;
      }
    }

    /* After processing sentences, retrieve satellite data */
    satellite_t local_satellites[gy_neo6mv2_max_satellites];
    uint8_t     satellite_count = priv_gy_neo6mv2_get_satellites(local_satellites, 
                                                                 gy_neo6mv2_max_satellites);

    /* Update sensor_data with satellite count */
    sensor_data->satellite_count = satellite_count;

    /* Process satellite data as needed */
    for (uint8_t i = 0; i < satellite_count; i++) {
      ESP_LOGI(gy_neo6mv2_tag, "Retrieved Satellite PRN=%d, Elevation=%d, Azimuth=%d, SNR=%d",
               local_satellites[i].prn, local_satellites[i].elevation,
               local_satellites[i].azimuth, local_satellites[i].snr);
    }

    return ESP_OK;
  } else {
    ESP_LOGE(gy_neo6mv2_tag, "Failed to read from GPS module");
    sensor_data->state = k_gy_neo6mv2_error;
    return ESP_FAIL;
  }
}

void gy_neo6mv2_reset_on_error(gy_neo6mv2_data_t *sensor_data)
{
  if (sensor_data->state == k_gy_neo6mv2_error) {
    TickType_t current_ticks = xTaskGetTickCount();

    if ((current_ticks - sensor_data->last_attempt_ticks) > sensor_data->retry_interval) {
      ESP_LOGI(gy_neo6mv2_tag, "Attempting to reset GY-NEO6MV2 GPS module");

      esp_err_t ret = gy_neo6mv2_init(sensor_data);
      if (ret == ESP_OK) {
        sensor_data->state          = k_gy_neo6mv2_ready;
        sensor_data->retry_count    = 0;
        sensor_data->retry_interval = gy_neo6mv2_initial_retry_interval;
        ESP_LOGI(gy_neo6mv2_tag, "GY-NEO6MV2 GPS module reset successfully.");
      } else {
        sensor_data->retry_count++;
        if (sensor_data->retry_count >= gy_neo6mv2_max_retries) {
          sensor_data->retry_count    = 0;
          sensor_data->retry_interval = (sensor_data->retry_interval * 2 > gy_neo6mv2_max_backoff_interval) ?
                                         gy_neo6mv2_max_backoff_interval : sensor_data->retry_interval * 2;
        }
      }

      sensor_data->last_attempt_ticks = current_ticks;
    }
  }
}

void gy_neo6mv2_tasks(void *sensor_data)
{
  gy_neo6mv2_data_t *gy_neo6mv2_data = (gy_neo6mv2_data_t *)sensor_data;
  while (1) {
    if (gy_neo6mv2_read(gy_neo6mv2_data) == ESP_OK) {
      char *json = gy_neo6mv2_data_to_json(gy_neo6mv2_data);
      send_sensor_data_to_webserver(json);
      file_write_enqueue("gy_neo6mv2.txt", json);
      free(json);
    } else {
      ESP_LOGW(gy_neo6mv2_tag, "Error reading GPS data, resetting...");
      gy_neo6mv2_reset_on_error(gy_neo6mv2_data);
    }
    vTaskDelay(gy_neo6mv2_polling_rate_ticks);
  }
}

