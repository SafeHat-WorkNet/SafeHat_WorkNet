#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>

#define MESH_PREFIX "SafeHatMesh"
#define MESH_PASSWORD "YourSecurePassword"
#define MESH_PORT 5555

const char* piSSID = "ESP_Mesh_Network";    // Raspberry Pi AP SSID
const char* piPassword = "YourSecurePassword"; // Raspberry Pi AP Password
const char* serverUrl = "http://192.168.4.1:5000/data"; // Flask server URL

Scheduler userScheduler;
painlessMesh mesh;

bool isBridge = false;      // Flag to indicate if this node is the bridge
bool meshStarted = false;   // Flag to indicate if the mesh has been started
bool serverReachable = false; // Flag to indicate if the Raspberry Pi server is reachable

// Node identification
uint32_t chipId = ESP.getEfuseMac(); // Unique chip ID
String nodeName = "Node" + String(chipId % 1000); // Short unique name (e.g., Node123)

// Task to send periodic messages within the mesh
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, []() {
  if (meshStarted) {
    String msg = "Hello from " + nodeName;
    Serial.printf("[%s][%s] Sending broadcast message: %s\n", 
      (isBridge ? "BRIDGE" : "NODE"), 
      nodeName.c_str(), 
      msg.c_str()
    );
    mesh.sendBroadcast(msg);
  }
});

// Task to check server connectivity and elect a bridge node
Task taskCheckServer(TASK_SECOND * 5, TASK_FOREVER, []() {
  if (!serverReachable) {
    Serial.printf("[%s] Checking server connectivity...\n", nodeName.c_str());

    // Attempt to connect to Raspberry Pi AP
    if (WiFi.status() != WL_CONNECTED) {
      Serial.printf("[%s] Connecting to Raspberry Pi AP...\n", nodeName.c_str());
      WiFi.begin(piSSID, piPassword);
      int retries = 0;
      while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
      }
      Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[%s] Connected to Raspberry Pi AP. Checking server...\n", nodeName.c_str());

      HTTPClient http;
      http.begin(serverUrl);
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.printf("[%s] Server reachable. HTTP response code: %d\n", nodeName.c_str(), httpResponseCode);
        serverReachable = true;
        isBridge = true;  // This node is now the bridge

        // Send an initial "Hello from {node id}" message to the server
        Serial.printf("[%s] Sending initial message to server...\n", nodeName.c_str());
        bool messageSent = false;
        int retries = 0;

        while (!messageSent && retries < 3) {
          HTTPClient httpPost;
          httpPost.begin(serverUrl);
          httpPost.addHeader("Content-Type", "application/json");

          String jsonData = "{\"node_id\": \"" + nodeName + "\", \"message\": \"Hello from " + nodeName + "\"}";
          int httpResponseCode = httpPost.POST(jsonData);

          if (httpResponseCode > 0) {
            Serial.printf("[%s] Initial message sent. Server response: %d\n", nodeName.c_str(), httpResponseCode);
            messageSent = true;
          } else {
            Serial.printf("[%s] Failed to send initial message: %s. Retrying...\n", nodeName.c_str(), http.errorToString(httpResponseCode).c_str());
            retries++;
            delay(1000); // Wait before retrying
          }

          httpPost.end();
        }

        if (!messageSent) {
          Serial.printf("[%s] Failed to send initial message after retries.\n", nodeName.c_str());
        }

        // Start the mesh network once the server is confirmed
        if (!meshStarted) {
          Serial.printf("[%s] Starting mesh network...\n", nodeName.c_str());
          int channel = WiFi.channel(); // Get Pi's AP channel
          mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, WIFI_AUTH_WPA2_PSK, channel);
          mesh.onReceive([](uint32_t from, String &msg) {
            Serial.printf("[%s] Received message from %u: %s\n", nodeName.c_str(), from, msg.c_str());

            // Forward messages to the server if this node is the bridge
            if (isBridge && serverReachable) {
              Serial.printf("[%s][BRIDGE] Forwarding message to server...\n", nodeName.c_str());
              HTTPClient http;
              http.begin(serverUrl);
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
          });
          mesh.onNewConnection([](uint32_t nodeId) {
            Serial.printf("[%s] New connection: %u\n", nodeName.c_str(), nodeId);
          });
          mesh.onChangedConnections([]() {
            Serial.printf("[%s] Mesh connections changed. Total nodes: %d\n", nodeName.c_str(), mesh.getNodeList().size());

            // If this node is the bridge and loses connection to the server, relinquish bridge role
            if (isBridge && !serverReachable) {
              Serial.printf("[%s][BRIDGE] Lost server connection. Relinquishing bridge role.\n", nodeName.c_str());
              isBridge = false;
            }

            // If no bridge exists, elect a new one
            if (!isBridge) {
              Serial.printf("[%s] Electing new bridge node...\n", nodeName.c_str());
              // Logic to elect a new bridge (e.g., based on node ID or signal strength)
              // For now, this node will take over as bridge
              isBridge = true;
              serverReachable = true;
            }
          });
          mesh.onNodeTimeAdjusted([](int32_t offset) {
            Serial.printf("[%s] Adjusted time %u. Offset = %d\n", nodeName.c_str(), mesh.getNodeTime(), offset);
          });
          mesh.onNodeDelayReceived([](uint32_t nodeId, int32_t delay) {
            Serial.printf("[%s] Delay to node %u is %d us\n", nodeName.c_str(), nodeId, delay);
          });
          meshStarted = true;
        }
      } else {
        Serial.printf("[%s] Server not reachable. HTTP error: %s\n", nodeName.c_str(), http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[%s] Failed to connect to Raspberry Pi AP.\n", nodeName.c_str());
    }
  }
});

// Task for the bridge to forward messages to the server
Task taskBridgeForward(TASK_SECOND * 10, TASK_FOREVER, []() {
  if (isBridge && serverReachable) {
    Serial.printf("[%s][BRIDGE] Sending bridge heartbeat...\n", nodeName.c_str());
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"node_id\": \"" + nodeName + "\", \"status\": \"bridge_heartbeat\"}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.printf("[%s][BRIDGE] Bridge heartbeat sent. Server response: %d\n", nodeName.c_str(), httpResponseCode);
    } else {
      Serial.printf("[%s][BRIDGE] Failed to send bridge heartbeat: %s\n", nodeName.c_str(), http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }
});

// Task to log mesh topology periodically
Task taskLogTopology(TASK_SECOND * 30, TASK_FOREVER, []() {
  Serial.printf("[%s] Connected nodes: %d\n", nodeName.c_str(), mesh.getNodeList().size());
});

// Setup function
void setup() {
  Serial.begin(115200);
  Serial.printf("[%s] Setup started...\n", nodeName.c_str());

  WiFi.mode(WIFI_STA); // Start in Station mode
  WiFi.disconnect();

  // Add tasks to the scheduler
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  userScheduler.addTask(taskCheckServer);
  taskCheckServer.enable();

  userScheduler.addTask(taskBridgeForward);
  taskBridgeForward.enable();

  userScheduler.addTask(taskLogTopology);
  taskLogTopology.enable();

  // Set debug message types (optional)
  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);

  Serial.printf("[%s] Setup completed.\n", nodeName.c_str());
}

// Loop function
void loop() {
  if (meshStarted) {
    mesh.update();
  }
  userScheduler.execute(); // Ensure scheduler runs
}