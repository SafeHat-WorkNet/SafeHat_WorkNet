#include "MeshNode.h"

// Static member initialization
MeshNode* MeshNode::instance = nullptr;
bool MeshNode::isBridge = false;
bool MeshNode::meshStarted = false;
bool MeshNode::serverReachable = false;
String MeshNode::nodeName = "";
painlessMesh MeshNode::mesh;
Scheduler MeshNode::userScheduler;
uint32_t MeshNode::currentBridgeId = 0;
int32_t MeshNode::bestRSSI = -1000;

// Constants initialization
const char* MeshNode::MESH_PREFIX = "SafeHatMesh";
const char* MeshNode::MESH_PASSWORD = "YourSecurePassword";
const int MeshNode::MESH_PORT = 5555;
const char* MeshNode::PI_SSID = "ESP_Mesh_Network";
const char* MeshNode::PI_PASSWORD = "YourSecurePassword";
const char* MeshNode::SERVER_URL = "http://192.168.4.1:5000/data";

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
    String role = isBridge ? "BRIDGE" : "NODE";
    String color = isBridge ? "\033[0;36m" : (level == "ERROR" ? "\033[0;31m" : "\033[0;32m");

    Serial.printf("%s[%s][%s][%s]\033[0m %s\n",
                  color.c_str(),
                  role.c_str(),
                  nodeName.c_str(),
                  level.c_str(),
                  message.c_str());
}

bool MeshNode::checkServerConnectivity() {
    HTTPClient http;
    http.begin(SERVER_URL);

    logMessage("Checking server connectivity...");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        logMessage("Server response: " + String(httpResponseCode));
        if (httpResponseCode == HTTP_CODE_OK) {
            String payload = http.getString();
            logMessage("Server payload: " + payload);
            http.end();
            return true;
        }
    } else {
        logMessage("Server connection failed: " + http.errorToString(httpResponseCode), "ERROR");
    }

    http.end();
    return false;
}

bool MeshNode::sendToServer(String jsonData) {
    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    logMessage("Sending to server: " + jsonData);
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
        String response = http.getString();
        logMessage("Server response: " + String(httpResponseCode) + " | " + response);
        http.end();
        return true;
    } else {
        logMessage("Failed to send data: " + http.errorToString(httpResponseCode), "ERROR");
        http.end();
        return false;
    }
}

String MeshNode::prepareNodeData() {
    DynamicJsonDocument doc(256);

    // Add node identifier
    doc["node"] = nodeName;

    // Add empty node-name (handled server-side)
    doc["node-name"] = "";

    // Set the type (root_node or child_node)
    doc["type"] = isBridge ? "root_node" : "child_node";

    // Add status (default to "Online" for now)
    JsonArray statusArray = doc.createNestedArray("status");
    statusArray.add("Online");

    // Serialize the JSON document to a string
    String jsonData;
    serializeJson(doc, jsonData);
    return jsonData;
}


void MeshNode::sendMessage() {
    if (!meshStarted) return;

    String nodeData = prepareNodeData();
    logMessage("Broadcasting node data: " + nodeData);
    mesh.sendBroadcast(nodeData);
}

void MeshNode::aggregateAndTransmitData() {
    if (!isBridge || !serverReachable) return;

    logMessage("Aggregating node data...");
    DynamicJsonDocument aggregatedDoc(1024);
    JsonArray nodesArray = aggregatedDoc.createNestedArray("nodes");

    for (auto& childData : receivedChildData) {
        DynamicJsonDocument tempDoc(256);
        DeserializationError error = deserializeJson(tempDoc, childData);
        if (!error) {
            nodesArray.add(tempDoc.as<JsonObject>());
        }
    }

    DynamicJsonDocument rootDoc(256);
    rootDoc["node"] = nodeName;
    rootDoc["node-name"] = "";
    rootDoc["type"] = "root_node";
    JsonArray rootStatus = rootDoc.createNestedArray("status");
    rootStatus.add("Online");

    nodesArray.add(rootDoc.as<JsonObject>());

    String aggregatedJson;
    serializeJson(aggregatedDoc, aggregatedJson);
    logMessage("Transmitting aggregated JSON: " + aggregatedJson);

    sendToServer(aggregatedJson);
    receivedChildData.clear();
}

void MeshNode::onReceiveCallback(uint32_t from, String &msg) {
    if (!instance) return;
    
    instance->logMessage("Received from " + String(from) + ": " + msg);

    if (!instance->isBridge) return;

    instance->receivedChildData.push_back(msg);
    instance->logMessage("Stored data from child node: " + msg);

    if (instance->receivedChildData.size() == instance->mesh.getNodeList().size() - 1) {
        instance->aggregateAndTransmitData();
    }
}

void MeshNode::onChangedConnectionsCallback() {
    if (!instance) return;
    instance->logMessage("Connections changed. Total nodes: " + String(mesh.getNodeList().size()));
}

void MeshNode::checkServer() {
    if (!isBridge) return;
    serverReachable = checkServerConnectivity();
    logMessage("Server " + String(serverReachable ? "is" : "is not") + " reachable");
}

void MeshNode::sendBridgeHeartbeat() {
    if (!isBridge || !serverReachable) return;
    String heartbeat = prepareNodeData();
    sendToServer(heartbeat);
}

void MeshNode::logTopology() {
    String status = String("Nodes: ") + mesh.getNodeList().size() +
                   " | Bridge: " + (isBridge ? nodeName : "None") +
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
