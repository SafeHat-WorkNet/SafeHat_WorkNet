#include "SensorManager.h"

SensorManager::SensorManager(MeshNode& node) : 
    meshNode(node),
    dht(DHT_PIN, DHT_TYPE)
{
    // Initialize sensor configurations
    sensors[0] = {
        "MPU6050",
        [this]() -> bool { 
            if (!mpu.begin()) {
                Serial.println("Failed to find MPU6050 chip");
                return false;
            }
            mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
            mpu.setGyroRange(MPU6050_RANGE_500_DEG);
            mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
            return true;
        },
        [this]() { readMPU(); },
        true,
        1000  // 1 second interval
    };
    
    sensors[1] = {
        "DHT22",
        [this]() -> bool {
            dht.begin();
            return true;
        },
        [this]() { readDHT(); },
        true,
        2000  // 2 second interval
    };
    
    sensors[2] = {
        "BH1750",
        [this]() -> bool {
            if (!lightMeter.begin()) {
                Serial.println("Failed to find BH1750 light sensor");
                return false;
            }
            return true;
        },
        [this]() { readLight(); },
        true,
        1000  // 1 second interval
    };
    
    // Create tasks
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensorTasks[i] = new Task(sensors[i].read_interval * TASK_MILLISECOND, TASK_FOREVER, 
                                sensors[i].read_function, &scheduler, true);
    }
}

void SensorManager::init() {
    Wire.begin();
    setupSensors();
    
    // Add and enable tasks
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensors[i].enabled) {
            scheduler.addTask(*sensorTasks[i]);
            sensorTasks[i]->enable();
        }
    }
}

void SensorManager::update() {
    scheduler.execute();
}

void SensorManager::setupSensors() {
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensors[i].enabled) {
            Serial.printf("Initializing sensor: %s\n", sensors[i].name);
            if (sensors[i].init_function()) {
                Serial.printf("Sensor %s initialized successfully\n", sensors[i].name);
            } else {
                Serial.printf("Sensor %s initialization failed\n", sensors[i].name);
                sensors[i].enabled = false;
            }
        } else {
            Serial.printf("Sensor %s is disabled\n", sensors[i].name);
        }
    }
}

void SensorManager::readMPU() {
    mpu.getEvent(&sensorData.accel, &sensorData.gyro, &sensorData.temp);
    sendSensorData();
}

void SensorManager::readDHT() {
    sensorData.temperature = dht.readTemperature();
    sensorData.humidity = dht.readHumidity();
    if (isnan(sensorData.temperature) || isnan(sensorData.humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    sendSensorData();
}

void SensorManager::readLight() {
    sensorData.light = lightMeter.readLightLevel();
    if (sensorData.light < 0) {
        Serial.println("Failed to read from BH1750 sensor!");
        return;
    }
    sendSensorData();
}

void SensorManager::sendSensorData() {
    StaticJsonDocument<200> doc;
    
    doc["node_id"] = meshNode.getNodeName();
    doc["temperature"] = sensorData.temperature;
    doc["humidity"] = sensorData.humidity;
    doc["light"] = sensorData.light;
    doc["accel_x"] = sensorData.accel.acceleration.x;
    doc["accel_y"] = sensorData.accel.acceleration.y;
    doc["accel_z"] = sensorData.accel.acceleration.z;
    doc["gyro_x"] = sensorData.gyro.gyro.x;
    doc["gyro_y"] = sensorData.gyro.gyro.y;
    doc["gyro_z"] = sensorData.gyro.gyro.z;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    MeshNode::getMesh().sendBroadcast(jsonString);
} 