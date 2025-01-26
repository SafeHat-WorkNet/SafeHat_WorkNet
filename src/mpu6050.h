#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>

class MPU6050 {
public:
    MPU6050(uint8_t addr = 0x68); // Default I2C address when AD0 is low
    bool begin();
    bool isReady();
    String getJsonString();
    
private:
    bool readData();
    void writeRegister(uint8_t reg, uint8_t data);
    uint8_t readRegister(uint8_t reg);
    
    uint8_t _i2cAddress;
    int16_t _accelX, _accelY, _accelZ;
    int16_t _gyroX, _gyroY, _gyroZ;
    int16_t _temp;
    unsigned long _lastReadTime;
    
    // MPU6050 Registers
    static const uint8_t MPU6050_PWR_MGMT_1 = 0x6B;
    static const uint8_t MPU6050_CONFIG = 0x1A;
    static const uint8_t MPU6050_GYRO_CONFIG = 0x1B;
    static const uint8_t MPU6050_ACCEL_CONFIG = 0x1C;
    static const uint8_t MPU6050_ACCEL_XOUT_H = 0x3B;
    
    // Scale factors
    static constexpr float ACCEL_SCALE = 16384.0; // For ±2g range
    static constexpr float GYRO_SCALE = 131.0;    // For ±250°/s range
    static constexpr float TEMP_SCALE = 340.0;    // Temperature conversion factor
    static constexpr float TEMP_OFFSET = 36.53;   // Temperature offset
};

#endif // MPU6050_H 