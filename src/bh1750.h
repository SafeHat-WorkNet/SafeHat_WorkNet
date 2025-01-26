#ifndef BH1750_H
#define BH1750_H

#include <Arduino.h>
#include <Wire.h>

class BH1750 {
public:
    // Device address options
    static const uint8_t ADDR_LOW = 0x23;  // ADDR pin low
    static const uint8_t ADDR_HIGH = 0x5C; // ADDR pin high

    // Measurement modes
    static const uint8_t MODE_CONTINUOUS_HIGH_RES = 0x10;   // 1 lx resolution
    static const uint8_t MODE_CONTINUOUS_HIGH_RES2 = 0x11;  // 0.5 lx resolution
    static const uint8_t MODE_CONTINUOUS_LOW_RES = 0x13;    // 4 lx resolution
    static const uint8_t MODE_ONE_TIME_HIGH_RES = 0x20;     // 1 lx resolution
    static const uint8_t MODE_ONE_TIME_HIGH_RES2 = 0x21;    // 0.5 lx resolution
    static const uint8_t MODE_ONE_TIME_LOW_RES = 0x23;      // 4 lx resolution

    // Power control commands
    static const uint8_t POWER_DOWN = 0x00;
    static const uint8_t POWER_ON = 0x01;
    static const uint8_t RESET = 0x07;

    // Constructor
    BH1750(uint8_t address = ADDR_LOW);

    // Initialization
    bool begin();

    // Configuration
    void setMode(uint8_t mode);
    void powerDown();
    void powerOn();
    void reset();

    // Measurement
    float readLightLevel();

private:
    uint8_t _address;
    uint8_t _mode;
    TwoWire* _wire;

    bool write(uint8_t data);
    uint16_t read();
};

#endif // BH1750_H
