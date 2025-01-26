#include "dht22.h"

DHT22Sensor::DHT22Sensor(uint8_t pin) : dht(pin, DHT22), lastReadTime(0), 
    lastTemperature(0.0f), lastHumidity(0.0f), sensorReady(false) {
}

void DHT22Sensor::begin() {
    dht.begin();
    sensorReady = true;
}

float DHT22Sensor::getTemperature() {
    updateReadings();
    return lastTemperature;
}

float DHT22Sensor::getHumidity() {
    updateReadings();
    return lastHumidity;
}

void DHT22Sensor::updateReadings() {
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime >= MIN_READ_INTERVAL) {
        lastTemperature = dht.readTemperature();
        lastHumidity = dht.readHumidity();
        lastReadTime = currentTime;
    }
}

String DHT22Sensor::getJsonString() {
    float temp = getTemperature();
    float hum = getHumidity();
    
    if (isnan(temp) || isnan(hum)) {
        return "{\"error\": \"Failed to read DHT22\"}";
    }
    
    return "{\"temperature\": " + String(temp, 1) + 
           ", \"humidity\": " + String(hum, 1) + "}";
}

bool DHT22Sensor::isReady() {
    return sensorReady;
} 