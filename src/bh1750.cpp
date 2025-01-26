#include "bh1750.h"

BH1750::BH1750(uint8_t address) {
    _address = address;
    _mode = MODE_CONTINUOUS_HIGH_RES;
    _wire = &Wire;
}

bool BH1750::begin() {
    _wire->begin();
    
    // Reset the device
    if (!write(POWER_ON) || !write(RESET)) {
        return false;
    }
    
    // Set default mode
    if (!write(_mode)) {
        return false;
    }
    
    return true;
}

void BH1750::setMode(uint8_t mode) {
    _mode = mode;
    write(mode);
}

void BH1750::powerDown() {
    write(POWER_DOWN);
}

void BH1750::powerOn() {
    write(POWER_ON);
}

void BH1750::reset() {
    powerOn();
    write(RESET);
}

float BH1750::readLightLevel() {
    uint16_t raw = read();
    
    // Convert raw value to lux
    // Each unit = 1/1.2 lux (datasheet)
    return (float)raw / 1.2;
}

bool BH1750::write(uint8_t data) {
    _wire->beginTransmission(_address);
    _wire->write(data);
    return (_wire->endTransmission() == 0);
}

uint16_t BH1750::read() {
    uint16_t value = 0;
    
    // Request 2 bytes from the sensor
    if (_wire->requestFrom(_address, (uint8_t)2) == 2) {
        value = _wire->read();      // MSB
        value <<= 8;
        value |= _wire->read();     // LSB
    }
    
    return value;
}
