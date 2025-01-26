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

void MeshNode::toggleLED() {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
}

void MeshNode::sendMessage() {
    if (meshStarted) {
        String msg = "Hello from " + nodeName;
        logMessage("Sending broadcast: " + msg);
        mesh.sendBroadcast(msg);
    }
}

void MeshNode::checkServer() {
    if (!serverReachable) {
        int32_t currentRSSI = WiFi.RSSI();
        
        if (currentRSSI > bestRSSI + 5) {
            bestRSSI = currentRSSI;
            
            if(WiFi.status() != WL_CONNECTED) {
                logMessage("Connecting to Pi AP... RSSI: " + String(currentRSSI) + " dBm");
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
                    currentBridgeId = mesh.getNodeId();
                    
                    String initData = "{\"node_id\": \"" + nodeName + "\", \"mac\": \"" + fullMac + "\", \"status\": \"online\"}";
                    if (sendToServer(initData)) {
                        logMessage("Successfully registered as bridge node");
                        mesh.sendBroadcast("BRIDGE_ELECT:" + String(mesh.getNodeId()) + ":" + String(bestRSSI));
                        
                        if (!meshStarted) {
                            int channel = WiFi.channel();
                            mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, WIFI_AUTH_WPA2_PSK, channel);
                            meshStarted = true;
                        }
                    }
                }
            }
        }
    }
}

void MeshNode::sendBridgeHeartbeat() {
    if (isBridge) {
        toggleLED();
        if (serverReachable) {
            String heartbeatData = "{\"node_id\": \"" + nodeName + "\", \"status\": \"heartbeat\", \"rssi\": " + String(WiFi.RSSI()) + "}";
            if (!sendToServer(heartbeatData)) {
                logMessage("Heartbeat failed, relinquishing bridge role", "ERROR");
                isBridge = false;
                serverReachable = false;
            }
        }
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}

void MeshNode::logTopology() {
    String status = String("Nodes: ") + mesh.getNodeList().size() +
                   " | RSSI: " + WiFi.RSSI() + " dBm" +
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

void MeshNode::onReceiveCallback(uint32_t from, String &msg) {
    if (!instance) return;
    
    instance->logMessage("Received from " + String(from) + ": " + msg);

    if (msg.startsWith("BRIDGE_ELECT")) {
        uint32_t senderId = msg.substring(12, msg.indexOf(":")).toInt();
        int32_t senderRSSI = msg.substring(msg.lastIndexOf(":")+1).toInt();
        
        if (senderRSSI > bestRSSI || (senderRSSI == bestRSSI && senderId < mesh.getNodeId())) {
            bestRSSI = senderRSSI;
            currentBridgeId = senderId;
            if(isBridge) {
                isBridge = false;
                serverReachable = false;
            }
        }
    }

    if (isBridge && serverReachable && !msg.startsWith("BRIDGE")) {
        String jsonData = "{\"from\": \"" + String(from) + "\", \"message\": \"" + msg + "\"}";
        if (!instance->sendToServer(jsonData)) {
            instance->logMessage("Failed to forward message to server", "ERROR");
        }
    }
}

void MeshNode::onChangedConnectionsCallback() {
    if (!instance) return;
    instance->logMessage("Connections changed. Total nodes: " + String(mesh.getNodeList().size()));
} 