#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <painlessMesh.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>

#define LED_PIN 2 // Replace '2' with your actual LED pin number

// Network Configuration
#define MESH_PREFIX "SafeHatMesh"
#define MESH_PASSWORD "YourSecurePassword"
#define MESH_PORT 5555

const char *piSSID = "ESP_Mesh_Network";
const char *piPassword = "YourSecurePassword";
const char *serverUrl = "http://192.168.4.1:5000/data";

// LED Root State
bool ledState = false; // Global variable to track LED state

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

// LED Control
void toggleLED() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
}

// Initialize Node Identity
void initNodeIdentity()
{
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    nodeName = "SafeHat-" + String(baseMac[4], HEX).substring(0, 2) +
               String(baseMac[5], HEX).substring(0, 2);
    fullMac = String(baseMac[0], HEX) + ":" + String(baseMac[1], HEX) + ":" +
              String(baseMac[2], HEX) + ":" + String(baseMac[3], HEX) + ":" +
              String(baseMac[4], HEX) + ":" + String(baseMac[5], HEX);
    chipId = ESP.getEfuseMac();
}

// Enhanced Logging System
void logMessage(String message, String level = "INFO")
{
    String role = isBridge ? "BRIDGE" : "NODE";
    String color = isBridge ? "\033[0;36m" : (level == "ERROR" ? "\033[0;31m" : "\033[0;32m");

    Serial.printf("%s[%s][%s][%s]\033[0m %s\n",
                  color.c_str(),
                  role.c_str(),
                  nodeName.c_str(),
                  level.c_str(),
                  message.c_str());
}

// Server Communication Functions
bool checkServerConnectivity()
{
    HTTPClient http;
    http.begin(serverUrl);

    logMessage("Checking server connectivity...");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
        logMessage("Server response: " + String(httpResponseCode));
        if (httpResponseCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            logMessage("Server payload: " + payload);
            http.end();
            return true;
        }
    }
    else
    {
        logMessage("Server connection failed: " + http.errorToString(httpResponseCode), "ERROR");
    }

    http.end();
    return false;
}

bool sendToServer(String jsonData)
{
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    logMessage("Sending to server: " + jsonData);
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        logMessage("Server response: " + String(httpResponseCode) + " | " + response);
        http.end();
        return true;
    }
    else
    {
        logMessage("Failed to send data: " + http.errorToString(httpResponseCode), "ERROR");
        http.end();
        return false;
    }
}

// Task: Periodic Mesh Messages
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, []()
                     {
  if (meshStarted) {
    String msg = "Hello from " + nodeName;
    logMessage("Sending broadcast: " + msg);
    mesh.sendBroadcast(msg);
  } });

// Task: Server Connectivity & Bridge Election
Task taskCheckServer(TASK_SECOND * 5, TASK_FOREVER, []()
                     {
  if (!serverReachable) {
    int32_t currentRSSI = WiFi.RSSI();
    
    if (currentRSSI > bestRSSI + 5) {
      bestRSSI = currentRSSI;
      
      if(WiFi.status() != WL_CONNECTED) {
        logMessage("Connecting to Pi AP... RSSI: " + String(currentRSSI) + " dBm");
        WiFi.begin(piSSID, piPassword);
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
          
          // Send initial node registration
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
  } });

// Task: Bridge Heartbeat
Task taskBridgeHeartbeat(TASK_SECOND * 1, TASK_FOREVER, []() {
  if (isBridge) {
    toggleLED(); // Flash LED when this node is the bridge
    if (serverReachable) {
      String heartbeatData = "{\"node_id\": \"" + nodeName + "\", \"status\": \"heartbeat\", \"rssi\": " + String(WiFi.RSSI()) + "}";
      if (!sendToServer(heartbeatData)) {
        logMessage("Heartbeat failed, relinquishing bridge role", "ERROR");
        isBridge = false;
        serverReachable = false;
      }
    }
  } else {
    digitalWrite(LED_PIN, LOW); // Ensure LED is off if not the bridge
  }
});

// Task: Network Status Reporting
Task taskLogTopology(TASK_SECOND * 30, TASK_FOREVER, []()
                     {
  String status = String("Nodes: ") + mesh.getNodeList().size() +
                 " | RSSI: " + WiFi.RSSI() + " dBm" +
                 " | Bridge: " + (isBridge ? nodeName : "None") +
                 " | IP: " + WiFi.localIP().toString();
  logMessage(status); });

// Mesh Event Handlers
void setupMeshCallbacks()
{
    mesh.onReceive([](uint32_t from, String &msg)
                   {
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
      String jsonData = "{\"from\": \"" + String(from) + "\", \"message\": \"" + msg + "\"}";
      if (!sendToServer(jsonData)) {
        logMessage("Failed to forward message to server", "ERROR");
      }
    } });

    mesh.onChangedConnections([]()
                              { logMessage("Connections changed. Total nodes: " + String(mesh.getNodeList().size())); });
}

void setup()
{
    // Root LED Identifier
    pinMode(LED_PIN, OUTPUT);   // Set the LED pin as output
    digitalWrite(LED_PIN, LOW); // Ensure LED is off initially

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

void loop()
{
    if (meshStarted)
    {
        mesh.update();
    }
    userScheduler.execute();
}