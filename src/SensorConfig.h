#pragma once

#include <functional>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <BH1750.h>
#include "SharedTypes.h"

// Forward declaration
class SensorManager;

struct SensorConfig {
    const char* name;                    // Sensor name for identification
    std::function<bool()> init_function; // Returns true if initialization successful
    std::function<void()> read_function; // Function to read sensor data
    bool enabled;                        // Whether the sensor is enabled
    uint32_t read_interval;             // Read interval in milliseconds
};

// Constants
constexpr uint8_t DHT_PIN = 4;  // GPIO4
constexpr uint8_t DHT_TYPE = DHT22; 