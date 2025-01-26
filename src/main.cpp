#include <Arduino.h>
#include "vfs_api.h"
#include "dht22.h"
#include "bh1750.h"
#include "ccs811.h"
#include "mpu6050.h"
#include "mq135.h"

DHT22Sensor dht22(4);
BH1750 lightSensor; // Using default I2C pins and address
CCS811 airSensor; // Using default I2C address
MPU6050 motionSensor; // Using default I2C address
MQ135 gasQualitySensor(34, 35); // AO=34, DO=35

void setup() {
    Serial.begin(115200);
    Serial.println("SafeHat WorkNet");
    
    Wire.begin(); // Initialize I2C
    dht22.begin();
    
    if (!lightSensor.begin()) {
        Serial.println("Could not initialize BH1750!");
    }
    
    if (!airSensor.begin()) {
        Serial.println("Could not initialize CCS811!");
    }
    
    if (!motionSensor.begin()) {
        Serial.println("Could not initialize MPU6050!");
    }
    
    if (!gasQualitySensor.begin()) {
        Serial.println("Could not initialize MQ135!");
    }
}

void loop() {
    if (dht22.isReady()) { 
        Serial.println(dht22.getJsonString()); 
    }
    
    // Read and format light sensor data as JSON
    float lux = lightSensor.readLightLevel();
    char jsonBuffer[64];
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"sensor\":\"BH1750\",\"light\":%.2f}", lux);
    Serial.println(jsonBuffer);
    
    // Read and print CCS811 data
    String airData = airSensor.getJsonString();
    if (airData.length() > 0) {
        Serial.println(airData);
    }
    
    // Read and print MPU6050 data
    String motionData = motionSensor.getJsonString();
    if (motionData.length() > 0) {
        Serial.println(motionData);
    }
    
    // Read and print MQ135 data
    String gasData = gasQualitySensor.getJsonString();
    if (gasData.length() > 0) {
        Serial.println(gasData);
    }
    
    delay(2000); // Wait for 2 seconds before next reading
}
