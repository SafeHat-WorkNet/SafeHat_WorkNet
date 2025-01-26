#include "MeshNode.h"

// Static member initialization
MeshNode* MeshNode::instance = nullptr;
bool MeshNode::isBridge = false;
bool MeshNode::meshStarted = false;
bool MeshNode::serverReachable = false;
String MeshNode::nodeName = "Node" + String(ESP.getEfuseMac() % 1000);
painlessMesh MeshNode::mesh;
Scheduler MeshNode::userScheduler;

const char* MeshNode::MESH_PREFIX = "SafeHatMesh";
const char* MeshNode::MESH_PASSWORD = "YourSecurePassword";
const int MeshNode::MESH_PORT = 5555;
const char* MeshNode::PI_SSID = "ESP_Mesh_Network";
const char* MeshNode::PI_PASSWORD = "YourSecurePassword";
const char* MeshNode::SERVER_URL = "http://192.168.4.1:5000/data";

MeshNode::MeshNode() {
    instance = this;
}

void MeshNode::init() {
    Serial.printf("[%s] Setup started...\n", nodeName.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    setupMesh();
}

void MeshNode::update() {
    if (meshStarted) {
        getMesh().update();
    }
}

void MeshNode::setupMesh() {
    getMesh().init(MESH_PREFIX, MESH_PASSWORD, &getScheduler(), MESH_PORT, WIFI_AP_STA, WIFI_AUTH_WPA2_PSK);
    getMesh().onReceive(&onReceiveCallback);
    getMesh().onNewConnection(&onNewConnectionCallback);
    getMesh().onChangedConnections(&onChangedConnectionsCallback);
    getMesh().onNodeTimeAdjusted(&onNodeTimeAdjustedCallback);
    getMesh().onNodeDelayReceived(&onNodeDelayReceivedCallback);
    meshStarted = true;
}

void MeshNode::onReceiveCallback(uint32_t from, String &msg) {
    Serial.printf("[%s] Received message from %u: %s\n", nodeName.c_str(), from, msg.c_str());

    if (isBridge && serverReachable) {
        Serial.printf("[%s][BRIDGE] Forwarding message to server...\n", nodeName.c_str());
        HTTPClient http;
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/json");

        String jsonData = "{\"from\": \"" + String(from) + "\", \"message\": \"" + msg + "\"}";
        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode > 0) {
            Serial.printf("[%s][BRIDGE] Forwarded message to server: %s\n", nodeName.c_str(), jsonData.c_str());
        } else {
            Serial.printf("[%s][BRIDGE] Failed to send message to server: %s\n", nodeName.c_str(), http.errorToString(httpResponseCode).c_str());
        }

        http.end();
    }
}

void MeshNode::onNewConnectionCallback(uint32_t nodeId) {
    Serial.printf("[%s] New connection: %u\n", nodeName.c_str(), nodeId);
}

void MeshNode::onChangedConnectionsCallback() {
    Serial.printf("[%s] Mesh connections changed. Total nodes: %d\n", 
        nodeName.c_str(), 
        getMesh().getNodeList().size()
    );
}

void MeshNode::onNodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("[%s] Adjusted time %u. Offset = %d\n", 
        nodeName.c_str(), 
        getMesh().getNodeTime(), 
        offset
    );
}

void MeshNode::onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay) {
    Serial.printf("[%s] Delay to node %u is %d us\n", nodeName.c_str(), nodeId, delay);
}

void MeshNode::checkServer() {
    if (!serverReachable) {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.begin(PI_SSID, PI_PASSWORD);
            delay(5000);
        }

        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(SERVER_URL);
            int httpResponseCode = http.GET();

            if (httpResponseCode > 0) {
                serverReachable = true;
                isBridge = true;
            }
            http.end();
        }
    }
}

void MeshNode::sendBridgeHeartbeat() {
    if (isBridge && serverReachable) {
        HTTPClient http;
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/json");

        String jsonData = "{\"node_id\": \"" + nodeName + "\", \"status\": \"bridge_heartbeat\"}";
        http.POST(jsonData);
        http.end();
    }
}

void MeshNode::logTopology() {
    Serial.printf("[%s] Connected nodes: %d\n", nodeName.c_str(), getMesh().getNodeList().size());
} 