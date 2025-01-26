#include "MeshNode.h"

// Static member initialization
MeshNode* MeshNode::instance = nullptr;
bool MeshNode::meshStarted = false;
String MeshNode::nodeName = "";
painlessMesh MeshNode::mesh;
Scheduler MeshNode::userScheduler;

// Constants initialization
const char* MeshNode::MESH_PREFIX = "SafeHatMesh";
const char* MeshNode::MESH_PASSWORD = "YourSecurePassword";
const int MeshNode::MESH_PORT = 5555;

MeshNode::MeshNode() {
    instance = this;
    ledState = false;
}

void MeshNode::init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    initNodeIdentity();
    logMessage("Initializing SafeHat node...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    setupMeshCallbacks();
    
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA);
    meshStarted = true;
    
    logMessage("Setup complete. Node ready.");
}

void MeshNode::update() {
    if (meshStarted) {
        mesh.update();
    }
}

void MeshNode::initNodeIdentity() {
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    nodeName = "SafeHat-" + String(baseMac[4], HEX).substring(0, 2) +
               String(baseMac[5], HEX).substring(0, 2);
    fullMac = String(baseMac[0], HEX) + ":" + String(baseMac[1], HEX) + ":" +
              String(baseMac[2], HEX) + ":" + String(baseMac[3], HEX) + ":" +
              String(baseMac[4], HEX) + ":" + String(baseMac[5], HEX);
    chipId = ESP.getEfuseMac();
}

void MeshNode::logMessage(String message, String level) {
    String color = (level == "ERROR" ? "\033[0;31m" : "\033[0;32m");
    Serial.printf("%s[NODE][%s][%s]\033[0m %s\n",
                  color.c_str(),
                  nodeName.c_str(),
                  level.c_str(),
                  message.c_str());
}

void MeshNode::toggleLED() {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
}

void MeshNode::logTopology() {
    String status = String("Nodes: ") + mesh.getNodeList().size() +
                   " | RSSI: " + WiFi.RSSI() + " dBm" +
                   " | IP: " + WiFi.localIP().toString();
    logMessage(status);
}

void MeshNode::setupMeshCallbacks() {
    mesh.onReceive([](uint32_t from, String &msg) {
        if (instance) {
            instance->onReceiveCallback(from, msg);
        }
    });

    mesh.onChangedConnections([]() {
        if (instance) {
            instance->onChangedConnectionsCallback();
        }
    });
}

void MeshNode::onReceiveCallback(uint32_t from, String &msg) {
    if (!instance) return;
    instance->logMessage("Received from " + String(from) + ": " + msg);
}

void MeshNode::onChangedConnectionsCallback() {
    if (!instance) return;
    instance->logMessage("Connections changed. Total nodes: " + String(mesh.getNodeList().size()));
} 