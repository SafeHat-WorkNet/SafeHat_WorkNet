#include "SensorManager.h"
#include <TaskScheduler.h>

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
    
    // Setup trigger pin
    pinMode(TRIGGER_PIN, INPUT);
    lastTriggerState = digitalRead(TRIGGER_PIN);
    
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
    checkTrigger();
}

void SensorManager::checkTrigger() {
    bool currentState = digitalRead(TRIGGER_PIN);
    
    // Detect rising edge (LOW to HIGH transition)
    if (currentState && !lastTriggerState) {
        // Read all sensors immediately
        readMPU();
        readDHT();
        readLight();
        // Send the data
        sendSensorData();
    }
    
    lastTriggerState = currentState;
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
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        // Set values to null (NAN in C++)
        sensorData.accelerometer.x = NAN;
        sensorData.accelerometer.y = NAN;
        sensorData.accelerometer.z = NAN;
        sensorData.gyroscope.x = NAN;
        sensorData.gyroscope.y = NAN;
        sensorData.gyroscope.z = NAN;
        return;
    }
    
    mpu.getEvent(&sensorData.accel, &sensorData.gyro, &sensorData.temp);
    sensorData.updateFromSensorEvents();
}

void SensorManager::readDHT() {
    float humidity = dht.readHumidity();
    float temp = dht.readTemperature();
    
    sensorData.humidity = isnan(humidity) ? NAN : humidity;
    sensorData.temperature = isnan(temp) ? NAN : temp;
}

void SensorManager::readLight() {
    if (!lightMeter.begin()) {
        Serial.println("Failed to find BH1750 light sensor");
        sensorData.light = NAN;
        return;
    }
    
    float light = lightMeter.readLightLevel();
    sensorData.light = light < 0 ? NAN : light;
    // Don't automatically send data here anymore
    // Only send when triggered or when all sensors have been read
}

void SensorManager::sendSensorData() {
    DynamicJsonDocument doc(512);
    doc["type"] = "sensor_data";
    doc["node_id"] = meshNode.getNodeName();
    doc["timestamp"] = millis();

    JsonObject data = doc.createNestedObject("data");
    
    // Only add values if they are not NAN
    if (!isnan(sensorData.temperature)) data["temperature"] = sensorData.temperature;
    if (!isnan(sensorData.humidity)) data["humidity"] = sensorData.humidity;
    if (!isnan(sensorData.light)) data["light"] = sensorData.light;

    // Add accelerometer data
    JsonObject accel = data.createNestedObject("accelerometer");
    if (!isnan(sensorData.accelerometer.x)) accel["x"] = sensorData.accelerometer.x;
    if (!isnan(sensorData.accelerometer.y)) accel["y"] = sensorData.accelerometer.y;
    if (!isnan(sensorData.accelerometer.z)) accel["z"] = sensorData.accelerometer.z;

    // Add gyroscope data
    JsonObject gyro = data.createNestedObject("gyroscope");
    if (!isnan(sensorData.gyroscope.x)) gyro["x"] = sensorData.gyroscope.x;
    if (!isnan(sensorData.gyroscope.y)) gyro["y"] = sensorData.gyroscope.y;
    if (!isnan(sensorData.gyroscope.z)) gyro["z"] = sensorData.gyroscope.z;

    String output;
    serializeJson(doc, output);
    meshNode.getMesh().sendBroadcast(output);
} 