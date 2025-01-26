#ifndef MQ135_H
#define MQ135_H

#include <Arduino.h>

class MQ135 {
public:
    MQ135(uint8_t analogPin, uint8_t digitalPin);
    bool begin();
    bool isReady();
    String getJsonString();
    float getPPM();
    void setRZero(float rzero); // Calibration function
    bool getDigitalValue(); // Get digital threshold status
    
private:
    bool readData();
    float getResistance();
    float getCorrectedResistance(float temp, float humidity);
    float getPPMFromResistance(float resistance);
    
    uint8_t _analogPin;
    uint8_t _digitalPin;
    float _rzero;
    float _correctedRZero;
    float _resistance;
    float _ppm;
    bool _digitalValue;
    unsigned long _lastReadTime;
    bool _isCalibrated;
    
    // Constants for air quality calculations
    static constexpr float RLOAD = 10.0;      // Load resistance in kΩ
    static constexpr float RZERO_CLEAN_AIR = 76.63; // R0 in clean air
    static constexpr float PARA = 116.6020682; // Curve parameters
    static constexpr float PARB = 2.769034857;
    
    static constexpr float CORA = 0.00035;    // Temperature/Humidity correction parameters
    static constexpr float CORB = 0.02718;
    static constexpr float CORC = 1.39538;
    static constexpr float CORD = 0.0018;
    
    static constexpr float ATMOCO2 = 397.13;  // Atmospheric CO2 level
};

#endif // MQ135_H 