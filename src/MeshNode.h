#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>

class MeshNode {
public:
    MeshNode();
    void init();
    void update();
    
    // Public methods
    void checkServer();
    void sendBridgeHeartbeat();
    void logTopology();
    void sendMessage();
    void toggleLED();
    String getNodeName() const { return nodeName; }
    bool sendToServer(String jsonData);
    bool checkServerConnectivity();
    void initNodeIdentity();
    void logMessage(String message, String level = "INFO");
    
    // Static methods for accessing mesh
    static painlessMesh& getMesh() { return mesh; }
    static Scheduler& getScheduler() { return userScheduler; }

    static void onReceiveCallback(uint32_t from, String &msg);
    static void onNewConnectionCallback(uint32_t nodeId);
    static void onChangedConnectionsCallback();
    static void onNodeTimeAdjustedCallback(int32_t offset);
    static void onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay);

private:
    static MeshNode* instance;
    static bool isBridge;
    static bool meshStarted;
    static bool serverReachable;
    static String nodeName;
    static painlessMesh mesh;
    static Scheduler userScheduler;
    static uint32_t currentBridgeId;
    static int32_t bestRSSI;
    
    // Node identification
    uint8_t baseMac[6];
    String fullMac;
    uint64_t chipId;
    bool ledState;
    
    void setupMesh();
    void setupMeshCallbacks();

    // Constants
    static const char* MESH_PREFIX;
    static const char* MESH_PASSWORD;
    static const int MESH_PORT;
    static const char* PI_SSID;
    static const char* PI_PASSWORD;
    static const char* SERVER_URL;
    static const int LED_PIN = 2;
}; 