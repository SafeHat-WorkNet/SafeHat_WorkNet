#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>

class MeshNode {
public:
    MeshNode();
    void init();
    void update();
    
    // Make these public for TaskManager
    void checkServer();
    void sendBridgeHeartbeat();
    void logTopology();
    String getNodeName() const { return nodeName; }
    
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
    
    void setupMesh();

    static const char* MESH_PREFIX;
    static const char* MESH_PASSWORD;
    static const int MESH_PORT;
    static const char* PI_SSID;
    static const char* PI_PASSWORD;
    static const char* SERVER_URL;
}; 