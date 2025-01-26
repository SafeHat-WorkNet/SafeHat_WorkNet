#include "MeshNode.h"

// Static member initialization
MeshNode* MeshNode::instance = nullptr;
bool MeshNode::meshStarted = false;
String MeshNode::nodeName = "";
painlessMesh MeshNode::mesh;
Scheduler MeshNode::userScheduler;
volatile bool MeshNode::triggerFlag = false;

// Constants initialization
const char* MeshNode::MESH_PREFIX = "SafeHatMesh";
const char* MeshNode::MESH_PASSWORD = "YourSecurePassword";
const int MeshNode::MESH_PORT = 5555;

MeshNode::MeshNode() : 
    ledState(false),
    dht(4)     // DHT22 on pin 4 (DHT_PIN from SensorConfig.h)
{
    instance = this;
}

void MeshNode::init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize trigger pin and interrupt
    setupInterrupt();
    
    // Initialize sensors
    Wire.begin();  // Start I2C bus
    dht.begin();
    
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
        // Check and clear the trigger flag if set
        if (triggerFlag) {
            sendSensorDataToServer();
            triggerFlag = false;
        }
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

void MeshNode::setupInterrupt() {
    pinMode(TRIGGER_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), handleTriggerInterrupt, RISING);
}

void IRAM_ATTR MeshNode::handleTriggerInterrupt() {
    triggerFlag = true;
}

void MeshNode::sendSensorDataToServer() {
    StaticJsonDocument<512> doc;
    doc["node"] = nodeName;
    doc["type"] = "sensor_data";
    doc["timestamp"] = mesh.getNodeTime();
    
    JsonObject sensorData = doc.createNestedObject("data");
    
    // Get sensor data from DHT22
    if (dht.isReady()) {
        String dhtData = dht.getJsonString();
        StaticJsonDocument<200> dhtDoc;
        deserializeJson(dhtDoc, dhtData);
        sensorData["dht22"] = dhtDoc.as<JsonObject>();
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send to all nodes (they will forward to server if they have connection)
    mesh.sendBroadcast(jsonString);
    logMessage("Sensor data sent: " + jsonString);
} 