#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>
#include <esp_log.h>
#include <vector>
#include <map>
#include "SharedTypes.h"

class MeshNode {
public:
    // Constants
    static constexpr unsigned long TOPOLOGY_LOG_INTERVAL = 30000; // 30 seconds
    static constexpr int LED_PIN = 2;
    
    MeshNode();
    void init();
    void update();
    
    // Public methods
    void checkServer();
    void logTopology();
    void toggleLED();
    String getNodeName() const { return nodeName; }
    bool sendToServer(String jsonData);
    bool checkServerConnectivity();
    void initNodeIdentity();
    void logMessage(String message, String level = "INFO");
    
    // Static methods for accessing mesh
    static painlessMesh& getMesh() { return mesh; }
    static Scheduler& getScheduler() { return userScheduler; }
    static bool isBridgeNode() { return isBridge; }
    static bool isServerReachable() { return serverReachable; }

    static void onReceiveCallback(uint32_t from, String &msg);
    static void onNewConnectionCallback(uint32_t nodeId);
    static void onChangedConnectionsCallback();
    static void onNodeTimeAdjustedCallback(int32_t offset);
    static void onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay);

    // New methods for event-based sending
    void sendNetworkUpdate();
    void sendRootNodeUpdate();
    void sendNodeStatusUpdate(const String& nodeId, const String& status);
    static String generateNetworkJson();
    
private:
    static MeshNode* instance;
    static bool isBridge;
    static bool meshStarted;
    static bool serverReachable;
    static String nodeName;
    static painlessMesh mesh;
    static Scheduler userScheduler;
    static String fullMac;
    
    // New members for tracking network state
    static uint32_t currentRootId;
    static std::vector<String> childrenIds;
    static std::vector<String> childrenStatus;
    static std::vector<String> childrenMacs;
    static bool networkStateChanged;
    
    // Event tracking
    static uint32_t lastRootId;
    static std::vector<String> lastChildrenIds;
    
    // Sensor data tracking
    static std::map<String, SensorData> latestSensorData;
    static bool sensorDataChanged;
    
    // Timing variables
    static unsigned long lastTopologyTime;
    
    // Constants
    static const char* MESH_PREFIX;
    static const char* MESH_PASSWORD;
    static const int MESH_PORT;
    static const char* PI_SSID;
    static const char* PI_PASSWORD;
    static const char* SERVER_URL;
    
    uint8_t baseMac[6];
    uint64_t chipId;
    bool ledState;
    
    void setupMesh();
    void setupMeshCallbacks();
    void updateSensorData(const String& nodeId, const SensorData& data);
}; 