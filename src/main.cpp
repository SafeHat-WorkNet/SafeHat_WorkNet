#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>
#include "MeshNode.h"
#include "TaskManager.h"
#include "SensorManager.h"

MeshNode meshNode;
TaskManager taskManager(meshNode);
SensorManager sensorManager(meshNode);

void setup() {
    Serial.begin(115200);
    meshNode.init();
    taskManager.init();
    sensorManager.init();
}

void loop() {
    meshNode.update();
    taskManager.execute();
    sensorManager.update();
}
