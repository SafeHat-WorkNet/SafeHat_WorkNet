#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>
#include "MeshNode.h"
#include "TaskManager.h"

MeshNode meshNode;
TaskManager taskManager(meshNode);

void setup() {
    Serial.begin(115200);
    meshNode.init();
    taskManager.init();
}

void loop() {
    meshNode.update();
    taskManager.execute();
}