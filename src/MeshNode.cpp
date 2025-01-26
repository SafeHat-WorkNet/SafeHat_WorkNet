#include "MeshNode.h"
#include <ArduinoJson.h>
#include <vector>

std::vector<String> receivedMessages;

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
const char* MeshNode::SERVER_URL = "http://192.168.1.2:5000/data";

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

    // Set a reasonable timeout
    http.setTimeout(10000);

    logMessage("Sending JSON to server: " + jsonData);
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
        if (isBridge) {
            logMessage("Compiling messages for server...");
            String compiledJson = "{\"nodes\": [";

            compiledJson += "{\"node\": \"" + nodeName + "\","
                            "\"node-name\": \"\","
                            "\"type\": \"root_node\","
                            "\"status\": [\"Online\"]},";

            for (const auto& msg : receivedMessages) {
                logMessage("Adding message: " + msg);
                compiledJson += msg + ",";
            }

            if (compiledJson.endsWith(",")) {
                compiledJson.remove(compiledJson.length() - 1);
            }
            compiledJson += "]}";

            logMessage("Compiled JSON: " + compiledJson);
            if (sendToServer(compiledJson)) {
                logMessage("Successfully sent compiled JSON to server");
                receivedMessages.clear();
            } else {
                logMessage("Failed to send compiled JSON to server", "ERROR");
            }
        } else {
            String jsonString = "{\"node\": \"" + nodeName + "\","
                                "\"node-name\": \"\","
                                "\"type\": \"child_node\","
                                "\"status\": [\"Online\"]}";
            logMessage("Broadcasting: " + jsonString);
            mesh.sendBroadcast(jsonString);
        }
    }
}


void MeshNode::onReceiveCallback(uint32_t from, String &msg) {
    if (instance) {
        instance->logMessage("Received from " + String(from, HEX) + ": " + msg);

        if (instance->isBridge) {
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, msg);
            if (!error) {
                receivedMessages.push_back(msg);
                instance->logMessage("Message stored: " + msg);
            } else {
                instance->logMessage("Invalid JSON received: " + msg, "ERROR");
            }
        }
    }
}

void MeshNode::checkServer() {
    if (!serverReachable) {
        logMessage("Checking server connection...");
        WiFi.disconnect();  // Ensure no stale connections
        WiFi.begin(PI_SSID, PI_PASSWORD);

        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
            logMessage("Connecting... Attempt: " + String(retries + 1));
            delay(500);
            retries++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            logMessage("Wi-Fi connected. RSSI: " + String(WiFi.RSSI()) + " dBm");
            if (checkServerConnectivity()) {
                serverReachable = true;
                isBridge = true;
                currentBridgeId = mesh.getNodeId();

                String initData = "{\"node_id\": \"" + nodeName + "\", \"mac\": \"" + fullMac + "\", \"status\": \"online\"}";
                if (sendToServer(initData)) {
                    logMessage("Successfully registered as bridge node.");
                    mesh.sendBroadcast("BRIDGE_ELECT:" + String(mesh.getNodeId()) + ":" + String(bestRSSI));

                    if (!meshStarted) {
                        int channel = WiFi.channel();
                        mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, WIFI_AUTH_WPA2_PSK, channel);
                        meshStarted = true;
                    }
                }
            }
        } else {
            logMessage("Wi-Fi connection failed after 20 attempts.", "ERROR");
        }
    }
}


void MeshNode::sendBridgeHeartbeat() {
    if (isBridge) {
        toggleLED();
        if (serverReachable) {
            String heartbeatData = "{\"node\": \"" + nodeName + "\", "
                       "\"type\": \"" + (isBridge ? "root_node" : "child_node") + "\", "
                       "\"status\": [\"Online\"]}";
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
            instance->logMessage("Connections changed. Total nodes: " + String(MeshNode::mesh.getNodeList().size()));
            instance->onChangedConnectionsCallback();
        }
    });
}

void MeshNode::onChangedConnectionsCallback() {
    if (instance) {
      instance->logMessage("Connections changed. Total nodes: " + String(mesh.getNodeList().size()));
    }
}