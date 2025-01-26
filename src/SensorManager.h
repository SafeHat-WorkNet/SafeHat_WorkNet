#pragma once

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <BH1750.h>
#include <ArduinoJson.h>
#include "MeshNode.h"
#include "SensorConfig.h"

// Forward declare TaskScheduler classes to avoid multiple definition errors
class Task;
class Scheduler;

class SensorManager {
public:
    static constexpr uint8_t TRIGGER_PIN = 13;  // D13 pin for triggering data send
    static constexpr int NUM_SENSORS = 3;
    
    SensorManager(MeshNode& node);
    void init();
    void update();

private:
    MeshNode& meshNode;
    Scheduler scheduler;
    
    // Sensor objects
    Adafruit_MPU6050 mpu;
    DHT dht;
    BH1750 lightMeter;
    
    // Sensor data
    SensorData sensorData;
    
    // Methods
    void setupSensors();
    void sendSensorData();
    
    // Sensor tasks
    void readMPU();
    void readDHT();
    void readLight();
    
    // Array of sensor configurations
    SensorConfig sensors[NUM_SENSORS];
    
    // Task array
    Task* sensorTasks[NUM_SENSORS];
    
    // Last trigger state
    bool lastTriggerState = false;
    
    // Methods
    void checkTrigger();
}; 