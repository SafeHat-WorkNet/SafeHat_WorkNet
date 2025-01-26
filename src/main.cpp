#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>
#include <esp_wifi.h>

// Network Configuration
#define MESH_PREFIX "SafeHatMesh"
#define MESH_PASSWORD "YourSecurePassword"
#define MESH_PORT 5555

const char* piSSID = "ESP_Mesh_Network";
const char* piPassword = "YourSecurePassword";
const char* serverUrl = "http://192.168.4.1:5000/data";

// Node Identification
uint8_t baseMac[6];
String nodeName;
String fullMac;
uint64_t chipId;

// System State
Scheduler userScheduler;
painlessMesh mesh;
bool isBridge = false;
bool meshStarted = false;
bool serverReachable = false;
uint32_t currentBridgeId = 0;
int32_t bestRSSI = -1000;

// Initialize Node Identity
void initNodeIdentity() {
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  nodeName = "SafeHat-" + String(baseMac[4], HEX).substring(0,2) + 
                        String(baseMac[5], HEX).substring(0,2);
  fullMac = String(baseMac[0], HEX) + ":" + String(baseMac[1], HEX) + ":" +
            String(baseMac[2], HEX) + ":" + String(baseMac[3], HEX) + ":" +
            String(baseMac[4], HEX) + ":" + String(baseMac[5], HEX);
  chipId = ESP.getEfuseMac();
}

// Enhanced Logging System
void logMessage(String message, String level = "INFO") {
  String role = isBridge ? "BRIDGE" : "NODE";
  String color = isBridge ? "\033[0;36m" : 
               (level == "ERROR" ? "\033[0;31m" : "\033[0;32m");
  
  Serial.printf("%s[%s][%s][%s]\033[0m %s\n",
    color.c_str(),
    role.c_str(),
    nodeName.c_str(),
    level.c_str(),
    message.c_str()
  );
}

// Task: Periodic Mesh Messages
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, []() {
  if (meshStarted) {
    String msg = "Hello from " + nodeName;
    logMessage("Sending broadcast: " + msg);
    mesh.sendBroadcast(msg);
  }
});

// Task: Server Connectivity & Bridge Election
Task taskCheckServer(TASK_SECOND * 5, TASK_FOREVER, []() {
  if (!serverReachable) {
    int32_t currentRSSI = WiFi.RSSI();
    
    if (currentRSSI > bestRSSI + 5) { // Hysteresis threshold
      bestRSSI = currentRSSI;
      if(WiFi.status() != WL_CONNECTED) {
        WiFi.begin(piSSID, piPassword);
        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
          delay(500);
          retries++;
        }
      }

      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
          serverReachable = true;
          isBridge = true;
          currentBridgeId = mesh.getNodeId();
          mesh.sendBroadcast("BRIDGE_ELECT:" + String(mesh.getNodeId()) + ":" + String(bestRSSI));
          
          if (!meshStarted) {
            int channel = WiFi.channel();
            mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, WIFI_AUTH_WPA2_PSK, channel);
            meshStarted = true;
          }
        }
        http.end();
      }
    }
  }
});

// Task: Bridge Heartbeat
Task taskBridgeHeartbeat(TASK_SECOND * 2, TASK_FOREVER, []() {
  if(isBridge) {
    mesh.sendBroadcast("BRIDGE_HB:" + String(mesh.getNodeId()));
  }
});

// Task: Network Status Reporting
Task taskLogTopology(TASK_SECOND * 30, TASK_FOREVER, []() {
  String status = String("Nodes: ") + mesh.getNodeList().size() +
                 " | RSSI: " + WiFi.RSSI() + " dBm" +
                 " | Bridge: " + (isBridge ? nodeName : "None") +
                 " | IP: " + WiFi.localIP().toString();
  logMessage(status);
});

// Mesh Event Handlers
void setupMeshCallbacks() {
  mesh.onReceive([](uint32_t from, String &msg) {
    logMessage("Received from " + String(from) + ": " + msg);

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
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");
      String jsonData = "{\"from\": \"" + String(from) + "\", \"message\": \"" + msg + "\"}";
      int httpResponseCode = http.POST(jsonData);
      http.end();
    }
  });

  mesh.onChangedConnections([]() {
    logMessage("Connections changed. Total nodes: " + String(mesh.getNodeList().size()));
  });
}

void setup() {
  Serial.begin(115200);
  initNodeIdentity();
  logMessage("Initializing SafeHat node...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize tasks
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskCheckServer);
  userScheduler.addTask(taskBridgeHeartbeat);
  userScheduler.addTask(taskLogTopology);
  
  taskSendMessage.enable();
  taskCheckServer.enable();
  taskBridgeHeartbeat.enable();
  taskLogTopology.enable();

  setupMeshCallbacks();
  
  logMessage("Setup complete. Node ready.");
}

void loop() {
  if (meshStarted) {
    mesh.update();
  }
  userScheduler.execute();
}