#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>

class MeshNode {
public:
    MeshNode();
    void init();
    void update();

    void checkServer();                // Missing method
    void sendBridgeHeartbeat();        // Missing method
    void logTopology();                // Missing method

    static void onChangedConnectionsCallback(); // Missing method

    void sendMessage();
    void toggleLED();
    String getNodeName() const { return nodeName; }
    bool sendToServer(String jsonData);
    bool checkServerConnectivity();
    void initNodeIdentity();
    void logMessage(String message, String level = "INFO");

    static painlessMesh& getMesh() { return mesh; }
    static Scheduler& getScheduler() { return userScheduler; }

    static void onReceiveCallback(uint32_t from, String &msg);

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

    uint8_t baseMac[6];
    String fullMac;
    uint64_t chipId;
    bool ledState;

    void setupMesh();
    void setupMeshCallbacks();

    String prepareNodeData();
    void aggregateAndTransmitData();

    std::vector<String> receivedChildData;

    static const char* MESH_PREFIX;
    static const char* MESH_PASSWORD;
    static const int MESH_PORT;
    static const char* PI_SSID;
    static const char* PI_PASSWORD;
    static const char* SERVER_URL;
    static const int LED_PIN = 2;
};
