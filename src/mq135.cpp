#include "mq135.h"

MQ135::MQ135(uint8_t analogPin, uint8_t digitalPin) : 
    _analogPin(analogPin), 
    _digitalPin(digitalPin),
    _rzero(RZERO_CLEAN_AIR), 
    _correctedRZero(RZERO_CLEAN_AIR),
    _resistance(0), 
    _ppm(0),
    _digitalValue(false),
    _lastReadTime(0), 
    _isCalibrated(false) {}

bool MQ135::begin() {
    pinMode(_analogPin, INPUT);
    pinMode(_digitalPin, INPUT);
    // Allow sensor to stabilize
    delay(1000);
    return true;
}

bool MQ135::isReady() {
    // Check if enough time has passed since last reading (minimum 100ms)
    if (millis() - _lastReadTime < 100) {
        return false;
    }
    return true;
}

float MQ135::getResistance() {
    int val = analogRead(_analogPin);
    // Convert analog reading to voltage (0-3.3V range for ESP32)
    float voltage = (float)val * (3.3 / 4095.0);
    // Calculate sensor resistance using voltage divider formula
    float resistance = ((3.3 * RLOAD) / voltage) - RLOAD;
    return resistance;
}

bool MQ135::getDigitalValue() {
    return digitalRead(_digitalPin) == HIGH;
}

float MQ135::getCorrectedResistance(float temp, float humidity) {
    // Get the base resistance
    float resistance = getResistance();
    
    // Correct for temperature and humidity
    float correction = CORA * temp * temp - CORB * temp + CORC - (humidity - 33.0) * CORD;
    return resistance / correction;
}

float MQ135::getPPMFromResistance(float resistance) {
    return PARA * pow((resistance / _rzero), -PARB);
}

void MQ135::setRZero(float rzero) {
    _rzero = rzero;
    _isCalibrated = true;
}

bool MQ135::readData() {
    _resistance = getResistance();
    _ppm = getPPMFromResistance(_resistance);
    _digitalValue = getDigitalValue();
    _lastReadTime = millis();
    return true;
}

float MQ135::getPPM() {
    if (isReady()) {
        readData();
    }
    return _ppm;
}

String MQ135::getJsonString() {
    if (isReady()) {
        if (!readData()) {
            return ""; // Return empty string on read error
        }
    }
    
    char jsonBuffer[128];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"sensor\":\"MQ135\","
             "\"gas_ppm\":%.2f,"
             "\"resistance\":%.2f,"
             "\"threshold_exceeded\":%s}",
             _ppm,
             _resistance,
             _digitalValue ? "true" : "false");
    
    return String(jsonBuffer);
} 