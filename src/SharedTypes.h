#pragma once

#include <Adafruit_Sensor.h>

// Structure to hold sensor data that will be used across the application
struct SensorData {
    // Raw sensor data
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    
    // Processed data for network transmission
    float temperature = 0;
    float humidity = 0;
    float light = 0;
    struct {
        float x = 0;
        float y = 0;
        float z = 0;
    } accelerometer;
    struct {
        float x = 0;
        float y = 0;
        float z = 0;
    } gyroscope;
    unsigned long timestamp = 0;

    // Update processed data from raw sensor data
    void updateFromSensorEvents() {
        accelerometer.x = accel.acceleration.x;
        accelerometer.y = accel.acceleration.y;
        accelerometer.z = accel.acceleration.z;
        
        gyroscope.x = gyro.gyro.x;
        gyroscope.y = gyro.gyro.y;
        gyroscope.z = gyro.gyro.z;
        
        temperature = temp.temperature;
        timestamp = millis();
    }
}; 