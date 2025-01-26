#include "MeshNode.h"
#include <ArduinoJson.h>

// Static member initialization
MeshNode* MeshNode::instance = nullptr;
bool MeshNode::isBridge = false;
bool MeshNode::meshStarted = false;
bool MeshNode::serverReachable = false;
String MeshNode::nodeName = "";
painlessMesh MeshNode::mesh;
Scheduler MeshNode::userScheduler;
String MeshNode::fullMac = "";

// New static member initialization
uint32_t MeshNode::currentRootId = 0;
uint32_t MeshNode::lastRootId = 0;
std::vector<String> MeshNode::childrenIds;
std::vector<String> MeshNode::childrenStatus;
std::vector<String> MeshNode::childrenMacs;
std::vector<String> MeshNode::lastChildrenIds;
bool MeshNode::networkStateChanged = false;

// Sensor data tracking initialization
std::map<String, SensorData> MeshNode::latestSensorData;
bool MeshNode::sensorDataChanged = false;

// Timing variables initialization
unsigned long MeshNode::lastTopologyTime = 0;

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
    digitalWrite(LED_PIN, LOW);  // Start with LED off
    
    initNodeIdentity();
    logMessage("Initializing SafeHat node...");

    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("http_client", ESP_LOG_ERROR);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    setupMesh();
    setupMeshCallbacks();
    
    meshStarted = true;
    logMessage("Setup complete. Node ready.");
}

void MeshNode::setupMesh() {
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}

void MeshNode::setupMeshCallbacks() {
    mesh.onReceive(&MeshNode::onReceiveCallback);
    mesh.onNewConnection(&MeshNode::onNewConnectionCallback);
    mesh.onChangedConnections(&MeshNode::onChangedConnectionsCallback);
    mesh.onNodeTimeAdjusted(&MeshNode::onNodeTimeAdjustedCallback);
    mesh.onNodeDelayReceived(&MeshNode::onNodeDelayReceivedCallback);
}

void MeshNode::update() {
    if (meshStarted) {
        mesh.update();
        
        // Check if we need to send a network update
        if (isBridge && (networkStateChanged || sensorDataChanged)) {
            sendNetworkUpdate();
            sensorDataChanged = false;
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
    if (!instance) return;
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
        http.end();
        return (httpResponseCode == HTTP_CODE_OK);
    } else {
        logMessage("Server connection failed: " + http.errorToString(httpResponseCode), "ERROR");
        http.end();
        return false;
    }
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

void MeshNode::toggleLED() {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
}

void MeshNode::checkServer() {
    if (!serverReachable) {
        if(WiFi.status() != WL_CONNECTED) {
            logMessage("Connecting to Pi AP...");
            WiFi.begin(PI_SSID, PI_PASSWORD);
            int retries = 0;
            while (WiFi.status() != WL_CONNECTED && retries < 20) {
                delay(500);
                retries++;
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            if (checkServerConnectivity()) {
                serverReachable = true;
                isBridge = true;
                currentRootId = mesh.getNodeId();
                digitalWrite(LED_PIN, HIGH);  // Turn on LED when becoming root
                
                // Send initial network state
                if (instance) {
                    instance->sendNetworkUpdate();
                }
                logMessage("Successfully registered as bridge node");
                
                if (!meshStarted) {
                    int channel = WiFi.channel();
                    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, WIFI_AUTH_WPA2_PSK, channel);
                    meshStarted = true;
                }
            }
        }
    }
}

void MeshNode::onReceiveCallback(uint32_t from, String &msg) {
    if (!instance) return;

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (!error) {
        if (doc.containsKey("type") && strcmp(doc["type"], "sensor_data") == 0) {
            String nodeId = doc["node_id"].as<String>();
            
            // Parse sensor data
            SensorData data;
            data.timestamp = doc["timestamp"] | 0;
            
            if (doc.containsKey("data")) {
                JsonObject sensorData = doc["data"];
                data.temperature = sensorData["temperature"] | 0.0f;
                data.humidity = sensorData["humidity"] | 0.0f;
                data.light = sensorData["light"] | 0;
                
                if (sensorData.containsKey("accelerometer")) {
                    data.accelerometer.x = sensorData["accelerometer"]["x"] | 0.0f;
                    data.accelerometer.y = sensorData["accelerometer"]["y"] | 0.0f;
                    data.accelerometer.z = sensorData["accelerometer"]["z"] | 0.0f;
                }
                
                if (sensorData.containsKey("gyroscope")) {
                    data.gyroscope.x = sensorData["gyroscope"]["x"] | 0.0f;
                    data.gyroscope.y = sensorData["gyroscope"]["y"] | 0.0f;
                    data.gyroscope.z = sensorData["gyroscope"]["z"] | 0.0f;
                }
                
                // Update sensor data and trigger status check
                instance->updateSensorData(nodeId, data);
            }
        }
    } else {
        instance->logMessage("Failed to parse JSON message: " + String(error.c_str()), "ERROR");
    }
}

void MeshNode::onChangedConnectionsCallback() {
    auto nodeList = mesh.getNodeList();
    
    // Clear current lists
    childrenIds.clear();
    childrenStatus.clear();
    childrenMacs.clear();
    
    // Update lists with current connections
    for (auto nodeId : nodeList) {
        String nodeIdStr = "SafeHat-" + String(nodeId);
        childrenIds.push_back(nodeIdStr);
        childrenStatus.push_back("Online");
        childrenMacs.push_back("0:0:0:0:0:0");
    }
    
    networkStateChanged = true;
    
    // If we're the bridge node, send an update
    if (isBridge && instance) {
        instance->sendNetworkUpdate();
    }
    
    if (instance) {
        instance->logMessage("Network topology changed. Connected nodes: " + String(mesh.getNodeList().size()));
    }
}

void MeshNode::onNewConnectionCallback(uint32_t nodeId) {
    if (!instance) return;
    instance->logMessage("New connection: Node " + String(nodeId));
    networkStateChanged = true;
}

void MeshNode::onNodeTimeAdjustedCallback(int32_t offset) {
    if (!instance) return;
    instance->logMessage("Time adjusted: " + String(offset) + " ms");
}

void MeshNode::onNodeDelayReceivedCallback(uint32_t nodeId, int32_t delay) {
    if (!instance) return;
    instance->logMessage("Delay to node " + String(nodeId) + ": " + String(delay) + "ms");
}

void MeshNode::updateSensorData(const String& nodeId, const SensorData& data) {
    latestSensorData[nodeId] = data;
    sensorDataChanged = true;
    
    // Check for potential fall or impact based on accelerometer data
    String status = "Online";
    if (abs(data.accelerometer.y) > 10 || abs(data.accelerometer.z) > 10) {
        status = "Alert - Impact";
    } else if (abs(data.accelerometer.y) > 8 || abs(data.accelerometer.z) > 8) {
        status = "Warning - Potential Fall";
    }
    
    // Update node status if needed
    if (isBridge) {
        sendNodeStatusUpdate(nodeId, status);
    }
}

String MeshNode::generateNetworkJson() {
    DynamicJsonDocument doc(2048);  // Increased size to accommodate sensor data
    
    doc["root-id"] = currentRootId == 0 ? "None" : "SafeHat-" + String(currentRootId);
    doc["root-status"] = serverReachable ? "Online" : "Offline";
    doc["root-mac"] = fullMac;
    
    JsonArray ids = doc.createNestedArray("children-ids");
    JsonArray statuses = doc.createNestedArray("children-status");
    JsonArray macs = doc.createNestedArray("children-macs");
    JsonObject sensorDataObj = doc.createNestedObject("sensor-data");
    
    for (size_t i = 0; i < childrenIds.size(); i++) {
        ids.add(childrenIds[i]);
        statuses.add(childrenStatus[i]);
        macs.add(childrenMacs[i]);
        
        // Add sensor data if available
        if (latestSensorData.count(childrenIds[i]) > 0) {
            const SensorData& data = latestSensorData[childrenIds[i]];
            JsonObject nodeData = sensorDataObj.createNestedObject(childrenIds[i]);
            
            nodeData["timestamp"] = data.timestamp;
            nodeData["temperature"] = data.temperature;
            nodeData["humidity"] = data.humidity;
            nodeData["light"] = data.light;
            
            JsonObject accel = nodeData.createNestedObject("accelerometer");
            accel["x"] = data.accelerometer.x;
            accel["y"] = data.accelerometer.y;
            accel["z"] = data.accelerometer.z;
            
            JsonObject gyro = nodeData.createNestedObject("gyroscope");
            gyro["x"] = data.gyroscope.x;
            gyro["y"] = data.gyroscope.y;
            gyro["z"] = data.gyroscope.z;
        }
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

void MeshNode::sendNetworkUpdate() {
    if (!isBridge) return;
    
    String json = generateNetworkJson();
    if (sendToServer(json)) {
        lastRootId = currentRootId;
        lastChildrenIds = childrenIds;
        networkStateChanged = false;
        logMessage("Network update sent successfully");
    }
}

void MeshNode::sendRootNodeUpdate() {
    if (!isBridge || currentRootId == lastRootId) return;
    
    String json = generateNetworkJson();
    if (sendToServer(json)) {
        lastRootId = currentRootId;
        logMessage("Root node update sent successfully");
    }
}

void MeshNode::sendNodeStatusUpdate(const String& nodeId, const String& status) {
    if (!isBridge) return;
    
    // Update the status for the node
    auto it = std::find(childrenIds.begin(), childrenIds.end(), nodeId);
    if (it != childrenIds.end()) {
        size_t index = std::distance(childrenIds.begin(), it);
        if (index < childrenStatus.size()) {
            // Only update and send if status has changed
            if (childrenStatus[index] != status) {
                childrenStatus[index] = status;
                networkStateChanged = true;
                String json = generateNetworkJson();
                if (sendToServer(json)) {
                    logMessage("Node status update sent for " + nodeId + ": " + status);
                }
            }
        }
    } else {
        // If node not found, add it to the lists
        childrenIds.push_back(nodeId);
        childrenStatus.push_back(status);
        childrenMacs.push_back("0:0:0:0:0:0"); // Default MAC for now
        networkStateChanged = true;
        String json = generateNetworkJson();
        if (sendToServer(json)) {
            logMessage("New node added and status sent for " + nodeId + ": " + status);
        }
    }
} 