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

// Task to send periodic messages within the mesh
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, []() {
  if (meshStarted) {
    String msg = "Hello from node: ";
    msg += mesh.getNodeId();
    Serial.printf("Sending broadcast message: %s\n", msg.c_str());
    mesh.sendBroadcast(msg);
  }
});

// Task to check server connectivity and elect a bridge node
Task taskCheckServer(TASK_SECOND * 5, TASK_FOREVER, []() {
  if (!serverReachable) {
    Serial.println("Checking server connectivity...");

    // Attempt to connect to Raspberry Pi AP
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connecting to Raspberry Pi AP...");
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
      Serial.println("Connected to Raspberry Pi AP. Checking server...");

      HTTPClient http;
      http.begin(serverUrl);
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.printf("Server reachable. HTTP response code: %d\n", httpResponseCode);
        serverReachable = true;
        isBridge = true;  // This node is now the bridge

        // Send an initial "Hello from {node id}" message to the server
        Serial.println("Sending initial message to server...");
        bool messageSent = false;
        int retries = 0;

        while (!messageSent && retries < 3) {
          HTTPClient httpPost;
          httpPost.begin(serverUrl);
          httpPost.addHeader("Content-Type", "application/json");

          String jsonData = "{\"node_id\": \"" + String(mesh.getNodeId()) + "\", \"message\": \"Hello from " + String(mesh.getNodeId()) + "\"}";
          int httpResponseCode = httpPost.POST(jsonData);

          if (httpResponseCode > 0) {
            Serial.printf("Initial message sent. Server response: %d\n", httpResponseCode);
            messageSent = true;
          } else {
            Serial.printf("Failed to send initial message: %s. Retrying...\n", http.errorToString(httpResponseCode).c_str());
            retries++;
            delay(1000); // Wait before retrying
          }

          httpPost.end();
        }

        if (!messageSent) {
          Serial.println("Failed to send initial message after retries.");
        }

        // Start the mesh network once the server is confirmed
        if (!meshStarted) {
          Serial.println("Starting mesh network...");
          int channel = WiFi.channel(); // Get Pi's AP channel
          mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, WIFI_AUTH_WPA2_PSK, channel);
          mesh.onReceive([](uint32_t from, String &msg) {
            Serial.printf("Received message from %u: %s\n", from, msg.c_str());

            // Forward messages to the server if this node is the bridge
            if (isBridge && serverReachable) {
              Serial.println("Forwarding message to server...");
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
          });
          mesh.onNewConnection([](uint32_t nodeId) {
            Serial.printf("New connection: %u\n", nodeId);
          });
          mesh.onChangedConnections([]() {
            Serial.println("Mesh connections changed");

            // If this node is the bridge and loses connection to the server, relinquish bridge role
            if (isBridge && !serverReachable) {
              Serial.println("Bridge node lost server connection. Relinquishing bridge role.");
              isBridge = false;
            }

            // If no bridge exists, elect a new one
            if (!isBridge) {
              Serial.println("Electing new bridge node...");
              // Logic to elect a new bridge (e.g., based on node ID or signal strength)
              // For now, this node will take over as bridge
              isBridge = true;
              serverReachable = true;
            }
          });
          mesh.onNodeTimeAdjusted([](int32_t offset) {
            Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
          });
          mesh.onNodeDelayReceived([](uint32_t nodeId, int32_t delay) {
            Serial.printf("Delay to node %u is %d us\n", nodeId, delay);
          });
          meshStarted = true;
        }
      } else {
        Serial.printf("Server not reachable. HTTP error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.println("Failed to connect to Raspberry Pi AP.");
    }
  }
});

// Task for the bridge to forward messages to the server
Task taskBridgeForward(TASK_SECOND * 10, TASK_FOREVER, []() {
  if (isBridge && serverReachable) {
    Serial.println("Sending bridge heartbeat...");
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

// Setup function
void setup() {
  Serial.begin(115200);
  Serial.println("Setup started...");

  WiFi.mode(WIFI_STA); // Start in Station mode
  WiFi.disconnect();

  // Add tasks to the scheduler
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  userScheduler.addTask(taskCheckServer);
  taskCheckServer.enable();

  userScheduler.addTask(taskBridgeForward);
  taskBridgeForward.enable();

  // Set debug message types (optional)
  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);

  Serial.println("Setup completed.");
}

// Loop function
void loop() {
  if (meshStarted) {
    mesh.update();
  }
  userScheduler.execute(); // Ensure scheduler runs
}