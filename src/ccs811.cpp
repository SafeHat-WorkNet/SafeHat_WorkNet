#include "ccs811.h"

CCS811::CCS811(uint8_t addr) : _i2cAddress(addr), _eco2(0), _tvoc(0), _lastReadTime(0) {}

bool CCS811::begin() {
    // Start I2C transaction
    Wire.beginTransmission(_i2cAddress);
    Wire.write(CCS811_APP_START);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    delay(20); // Wait for the device to be ready
    
    // Configure measurement mode (1 second interval)
    Wire.beginTransmission(_i2cAddress);
    Wire.write(CCS811_MEAS_MODE);
    Wire.write(0x10); // Constant power mode, measurement every second
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    return true;
}

bool CCS811::isReady() {
    // Check if enough time has passed since last reading (1 second minimum)
    if (millis() - _lastReadTime < 1000) {
        return false;
    }
    
    // Read status register
    Wire.beginTransmission(_i2cAddress);
    Wire.write(CCS811_STATUS);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    Wire.requestFrom(_i2cAddress, (uint8_t)1);
    if (Wire.available()) {
        uint8_t status = Wire.read();
        // Check if data is ready (bit 3)
        return (status & 0x08) != 0;
    }
    
    return false;
}

bool CCS811::readData() {
    Wire.beginTransmission(_i2cAddress);
    Wire.write(CCS811_ALG_RESULT_DATA);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    // Read 4 bytes of data
    if (Wire.requestFrom(_i2cAddress, (uint8_t)4) != 4) {
        return false;
    }
    
    _eco2 = (Wire.read() << 8) | Wire.read();
    _tvoc = (Wire.read() << 8) | Wire.read();
    _lastReadTime = millis();
    
    return true;
}

String CCS811::getJsonString() {
    if (isReady()) {
        if (!readData()) {
            return ""; // Return empty string on read error
        }
    }
    
    char jsonBuffer[64];
    snprintf(jsonBuffer, sizeof(jsonBuffer), 
             "{\"sensor\":\"CCS811\",\"eCO2\":%u,\"TVOC\":%u}", 
             _eco2, _tvoc);
    return String(jsonBuffer);
} 