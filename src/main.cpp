#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <SPIFFS.h>
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include <painlessMesh.h> 

#define MESH_PREFIX "ESP_Mesh_NetworkS"
#define MESH_PASSWORD "YourSecurePassword"
#define MESH_PORT 5555

const char* piSSID = "RaspberryPi_SSID"; // Raspberry Pi AP SSID
const char* piPassword = "YourP iPassword"; // Raspberry Pi AP Password
const char* serverUrl = "http://192.168.1.2:5555/data"; // Flask server URL

Scheduler userScheduler;
painlessMesh mesh;

bool isBridge = false; // Flag to indicate if this node is the bridge
bool bridgeActive = false; // Flag to indicate if a bridge is currently active
uint32_t currentBridgeId = 0; // Tracks the current bridge node ID

// Task to periodically send messages within the mesh
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, []() {
  String msg = "Hello from node: ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
});

// Task to monitor the bridge status
Task taskMonitorBridge(TASK_SECOND * 5, TASK_FOREVER, []() {
  if (!bridgeActive || currentBridgeId == 0) {
    // No bridge is active, self-assign as bridge if this node has the lowest ID
    std::list<uint32_t> nodeList = mesh.getNodeList();
    nodeList.sort(); // Sort the node IDs
    uint32_t lowestId = *nodeList.begin(); // Get the lowest node ID

    if (mesh.getNodeId() == lowestId) {
      // This node becomes the bridge
      Serial.println("No active bridge detected. Becoming the bridge...");
      WiFi.begin(piSSID, piPassword);

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to Raspberry Pi AP. Broadcasting bridge status...");
        isBridge = true;
        currentBridgeId = mesh.getNodeId();
        bridgeActive = true;
        mesh.sendBroadcast("BRIDGE_ACTIVE");
      } else {
        Serial.println("Failed to connect to Raspberry Pi AP.");
      }
    }
  }
});

// Task for the bridge to send periodic updates to the Flask server
Task taskBridgeHeartbeat(TASK_SECOND * 10, TASK_FOREVER, []() {
  if (isBridge && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"node_id\": \"" + String(mesh.getNodeId()) + "\", \"status\": \"bridge_heartbeat\"}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.printf("Bridge heartbeat sent. Server response: %d\n", httpResponseCode);
    } else {
      Serial.printf("Failed to send bridge heartbeat: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }
});

// Callback for receiving mesh messages
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received message from %u: %s\n", from, msg.c_str());

  if (msg == "BRIDGE_ACTIVE") {
    bridgeActive = true;
    currentBridgeId = from; // Update the current bridge ID
  } else if (isBridge) {
    // Forward messages to the Flask server if this node is the bridge
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"from\": \"" + String(from) + "\", \"message\": \"" + msg + "\"}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.printf("Forwarded message to server: %s\n", jsonData.c_str());
    } else {
      Serial.printf("Failed to send message to server: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }
}

// Callback for mesh connection changes
void changedConnectionCallback() {
  Serial.println("Mesh connections changed");

  // Re-evaluate bridge status
  bridgeActive = false;
  currentBridgeId = 0;
  mesh.sendBroadcast("BRIDGE_CHECK");
}

// Setup function
void setup() {
  Serial.begin(115200);

  // Initialize the mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  // Add tasks to the scheduler
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  userScheduler.addTask(taskMonitorBridge);
  taskMonitorBridge.enable();

  userScheduler.addTask(taskBridgeHeartbeat);
  taskBridgeHeartbeat.enable();
}

// Loop function
void loop() {
  mesh.update();
}
