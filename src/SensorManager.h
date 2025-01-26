#pragma once

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <BH1750.h>
#include <ArduinoJson.h>
#include "MeshNode.h"
#include "SensorConfig.h"

class SensorManager {
public:
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
    static const int NUM_SENSORS = 3;
    SensorConfig sensors[NUM_SENSORS];
    
    // Task array
    Task* sensorTasks[NUM_SENSORS];
}; 