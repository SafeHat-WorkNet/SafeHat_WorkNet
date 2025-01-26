#ifndef DHT22_H
#define DHT22_H

#include <Arduino.h>
#include <DHT.h>

class DHT22Sensor {
public:
    DHT22Sensor(uint8_t pin);
    void begin();
    float getTemperature();
    float getHumidity();
    String getJsonString();
    bool isReady();

private:
    void updateReadings();
    DHT dht;
    unsigned long lastReadTime;
    static const unsigned long MIN_READ_INTERVAL = 2000; // Minimum time between reads (2 seconds)
    float lastTemperature;
    float lastHumidity;
    bool sensorReady;
};

#endif // DHT22_H 