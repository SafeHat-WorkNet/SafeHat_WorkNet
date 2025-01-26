#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <painlessMesh.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "dht22.h"

class MeshNode {
public:
    MeshNode();
    void init();
    void update();
    
    // Public methods
    void logTopology();
    void toggleLED();
    String getNodeName() const { return nodeName; }
    void initNodeIdentity();
    void logMessage(String message, String level = "INFO");
    void sendSensorDataToServer();
    
    // Static methods for accessing mesh
    static painlessMesh& getMesh() { return mesh; }
    static Scheduler& getScheduler() { return userScheduler; }

    static void onReceiveCallback(uint32_t from, String &msg);
    static void onNewConnectionCallback(uint32_t nodeId);
    static void onChangedConnectionsCallback();
    static void onNodeTimeAdjustedCallback(int32_t offset);
    static void onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay);
    
    // Interrupt handler
    static void IRAM_ATTR handleTriggerInterrupt();

private:
    static MeshNode* instance;
    static bool meshStarted;
    static String nodeName;
    static painlessMesh mesh;
    static Scheduler userScheduler;
    static volatile bool triggerFlag;  // Flag set by interrupt handler
    
    // Node identification
    uint8_t baseMac[6];
    String fullMac;
    uint64_t chipId;
    bool ledState;
    
    // Sensors
    DHT22Sensor dht;
    
    void setupMesh();
    void setupMeshCallbacks();
    void setupInterrupt();

    // Constants
    static const char* MESH_PREFIX;
    static const char* MESH_PASSWORD;
    static const int MESH_PORT;
    static const int LED_PIN = 2;
    static const int TRIGGER_PIN = 13;
}; 