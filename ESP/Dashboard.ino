#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#include "html.h"

const char* AP_SSID = "CSS";
const char* AP_PASS = "css2026";

#define SERIAL2_RX 16
#define SERIAL2_TX 17

WebServer server(80);
WebSocketsServer webSocket(81);

// Machine state
struct {
  int colorCounts[5] = { 0, 0, 0, 0, 0 };
  int totalSorted = 0;
  String currentState = "";
  String lastColor = "unknown";
  unsigned long uptimeStart = 0;
} MachineData;

// Controls
struct {
  int speed = 50;
  String mode = "auto";
} controls;

// Read JSON from Arduino
void readPin() {
  if (!Serial2.available()) return;

  String line = Serial2.readStringUntil('\n');
  line.trim();
  if (!line.length()) return;

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    Serial.print("Serial2 JSON error: ");
    Serial.println(err.c_str());
    Serial.println(line);
    return;
  }

  const char* type = doc["type"] | "";

  if (strcmp(type, "state") == 0) {
    MachineData.currentState = doc["state"] | "idle";
    return;
  }
  if (strcmp(type, "detection_final") == 0) {
    const char* color = doc["color"] | "unknown";
    MachineData.lastColor = color;
    if      (strcmp(color, "RED")    == 0) MachineData.colorCounts[0]++;
    else if (strcmp(color, "GREEN")  == 0) MachineData.colorCounts[1]++;
    else if (strcmp(color, "BLUE")   == 0) MachineData.colorCounts[2]++;
    else if (strcmp(color, "YELLOW") == 0) MachineData.colorCounts[3]++;
    else if (strcmp(color, "BROWN")  == 0) MachineData.colorCounts[4]++;
    MachineData.totalSorted++;
    return;
  }
}

// Update Dashboard
void updateDashboard() {
  StaticJsonDocument<512> doc;

  JsonObject colors = doc.createNestedObject("colors");
  colors["red"]    = MachineData.colorCounts[0];
  colors["green"]  = MachineData.colorCounts[1];
  colors["blue"]   = MachineData.colorCounts[2];
  colors["yellow"] = MachineData.colorCounts[3];
  colors["brown"]  = MachineData.colorCounts[4];

  JsonObject stats = doc.createNestedObject("stats");
  stats["totalSorted"] = MachineData.totalSorted;

  unsigned long uptimeSec = (millis() - MachineData.uptimeStart) / 1000;
  stats["uptime"]    = uptimeSec;
  stats["sortRate"]  = 0;

  doc["status"] = MachineData.currentState;

  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);
}

// Send an action
void sendAction(StaticJsonDocument<256>& doc) {
  String json;
  serializeJson(doc, json);
  Serial2.println(json);
  Serial.println(json);
}

// WebSocket event handler
void onWebSocketEvent(uint8_t client, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("WS client %u connected\n", client);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("WS client %u disconnected\n", client);
  } else if (type == WStype_TEXT) {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) return;

    if (doc.containsKey("action")) {
      sendAction(doc);
    }
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // 192.168.4.1

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println("WebSocket server started on port 81");

  MachineData.uptimeStart = millis();
}

// Loop
void loop() {
  server.handleClient();
  webSocket.loop();
  readPin();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 500) {
    lastSend = millis();
    updateDashboard();
  }
}
