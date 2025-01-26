/* components/sensors/include/sensor_hal.h */

#ifndef SAFEHAT_WORKNET_SENSOR_HAL_H
#define SAFEHAT_WORKNET_SENSOR_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dht22_hal.h"
#include "ccs811_hal.h"
#include "mq135_hal.h"
#include "gy_neo6mv2_hal.h"
#include "bh1750_hal.h"
#include "mpu6050_hal.h"

/* Structs ********************************************************************/

/**
 * @brief Structure to store data from various connected sensors.
 *
 * Contains the recorded data from all connected sensors, including light intensity,
 * temperature, humidity, gyroscope, accelerometer, magnetometer, air quality, and GPS.
 * Each field represents the data collected from a specific sensor type.
 */
typedef struct {
  dht22_data_t      dht22_data;      /**< Data from the DHT22 temperature and humidity sensor. */
  ccs811_data_t     ccs811_data;     /**< Data from the CCS811 air quality sensor (eCO2 and TVOC levels). */
  mq135_data_t      mq135_data;      /**< Data from the MQ135 air quality sensor (e.g., CO2, ammonia, benzene). */
  gy_neo6mv2_data_t gy_neo6mv2_data; /**< Data from the GY-NEO6MV2 GPS sensor. */
  bh1750_data_t     bh1750_data;     /**< Data from the BH1750 light intensity sensor. */
  mpu6050_data_t    mpu6050_data;    /**< Data from the MPU6050 motion sensor. */
} sensor_data_t;

#ifdef __cplusplus
}
#endif

#endif /* SAFEHAT_WORKNET_SENSOR_HAL_H */
