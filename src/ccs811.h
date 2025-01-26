#ifndef CCS811_H
#define CCS811_H

#include <Arduino.h>
#include <Wire.h>

class CCS811 {
public:
    CCS811(uint8_t addr = 0x5A); // Default I2C address
    bool begin();
    bool isReady();
    String getJsonString();
    
private:
    bool readData();
    uint8_t _i2cAddress;
    uint16_t _eco2;
    uint16_t _tvoc;
    unsigned long _lastReadTime;
    static const uint8_t CCS811_STATUS = 0x00;
    static const uint8_t CCS811_MEAS_MODE = 0x01;
    static const uint8_t CCS811_ALG_RESULT_DATA = 0x02;
    static const uint8_t CCS811_APP_START = 0xF4;
};

#endif // CCS811_H 