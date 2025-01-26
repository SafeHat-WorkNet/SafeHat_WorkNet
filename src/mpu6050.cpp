#include "mpu6050.h"

MPU6050::MPU6050(uint8_t addr) : _i2cAddress(addr), _accelX(0), _accelY(0), _accelZ(0),
                                 _gyroX(0), _gyroY(0), _gyroZ(0), _temp(0), _lastReadTime(0) {}

void MPU6050::writeRegister(uint8_t reg, uint8_t data) {
    Wire.beginTransmission(_i2cAddress);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

uint8_t MPU6050::readRegister(uint8_t reg) {
    Wire.beginTransmission(_i2cAddress);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_i2cAddress, (uint8_t)1);
    return Wire.read();
}

bool MPU6050::begin() {
    // Wake up the MPU6050 by writing 0 to the power management register
    writeRegister(MPU6050_PWR_MGMT_1, 0x00);
    
    // Configure the accelerometer for ±2g range
    writeRegister(MPU6050_ACCEL_CONFIG, 0x00);
    
    // Configure the gyroscope for ±250°/s range
    writeRegister(MPU6050_GYRO_CONFIG, 0x00);
    
    // Set sample rate divider and digital low pass filter
    writeRegister(MPU6050_CONFIG, 0x03); // DLPF at 44Hz
    
    // Verify the device ID (should be 0x68)
    uint8_t whoAmI = readRegister(0x75);
    return (whoAmI == 0x68);
}

bool MPU6050::isReady() {
    // Check if enough time has passed since last reading (minimum 10ms)
    if (millis() - _lastReadTime < 10) {
        return false;
    }
    return true;
}

bool MPU6050::readData() {
    Wire.beginTransmission(_i2cAddress);
    Wire.write(MPU6050_ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    
    // Request 14 bytes (6 accel + 2 temp + 6 gyro)
    if (Wire.requestFrom(_i2cAddress, (uint8_t)14) != 14) {
        return false;
    }
    
    // Read accelerometer data
    _accelX = (Wire.read() << 8) | Wire.read();
    _accelY = (Wire.read() << 8) | Wire.read();
    _accelZ = (Wire.read() << 8) | Wire.read();
    
    // Read temperature data
    _temp = (Wire.read() << 8) | Wire.read();
    
    // Read gyroscope data
    _gyroX = (Wire.read() << 8) | Wire.read();
    _gyroY = (Wire.read() << 8) | Wire.read();
    _gyroZ = (Wire.read() << 8) | Wire.read();
    
    _lastReadTime = millis();
    return true;
}

String MPU6050::getJsonString() {
    if (isReady()) {
        if (!readData()) {
            return ""; // Return empty string on read error
        }
    }
    
    // Convert raw values to actual measurements
    float accelX = _accelX / ACCEL_SCALE;
    float accelY = _accelY / ACCEL_SCALE;
    float accelZ = _accelZ / ACCEL_SCALE;
    
    float gyroX = _gyroX / GYRO_SCALE;
    float gyroY = _gyroY / GYRO_SCALE;
    float gyroZ = _gyroZ / GYRO_SCALE;
    
    float tempC = (_temp / TEMP_SCALE) + TEMP_OFFSET;
    
    char jsonBuffer[256];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"sensor\":\"MPU6050\","
             "\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
             "\"gyro\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
             "\"temp\":%.1f}",
             accelX, accelY, accelZ,
             gyroX, gyroY, gyroZ,
             tempC);
    
    return String(jsonBuffer);
} 