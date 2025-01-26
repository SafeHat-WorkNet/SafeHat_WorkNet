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

#define   MESH_PREFIX     "ESP_Mesh_Network"
#define   MESH_PASSWORD   "YourSecurePassword"
#define   MESH_PORT       5555

const char* ssid = MESH_PREFIX;
const char* password = MESH_PASSWORD;
const char* serverUrl = "http://192.168.1.2:5555/data"; // Update with Raspberry Pi's IP address

Scheduler userScheduler;
painlessMesh  mesh;

// User stub
void sendMessage(); // Prototype so PlatformIO doesn't complain
void sendPostRequest();

Task taskSendMessage(TASK_SECOND * 1 , TASK_FOREVER, &sendMessage);
Task taskSendPostRequest(TASK_SECOND * 10, TASK_FOREVER, &sendPostRequest);

void sendMessage() {
  String msg = "Hi from node1";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void sendPostRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"message\": \"Hello, ESP!\"}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("HTTP Response code: %d\nResponse: %s\n", httpResponseCode, response.c_str());
    } else {
      Serial.printf("Error in sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Mesh network setup
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  userScheduler.addTask(taskSendPostRequest);
  taskSendPostRequest.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}