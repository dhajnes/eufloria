#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Replace these with your own WiFi network credentials
const char* ssid = "Hotspot";
const char* password = "SPDPassword";

// The IP address and port number of the remote server
const char* serverIP = "10.42.0.1";
const int serverPort = 1880;
const char* urlPath = "/url";

WiFiClient client;
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up the local server
  server.on("/esp", handleIncomingRequest);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle incoming requests
  server.handleClient();

  // Send POST request to the remote server
  sendPostRequest();

  // Wait for 30 seconds before sending the next request
  delay(30000);
}

void sendPostRequest() {
  if (!client.connect(serverIP, serverPort)) {
    Serial.println("Connection failed");
    return;
  }

  // Prepare JSON payload
//  StaticJsonDocument<128> jsonDoc;
//  JsonArray tempArray = jsonDoc.createNestedArray("temp");
//  tempArray.add(0);
//  tempArray.add(2);
//  tempArray.add(2);
  StaticJsonDocument<256> jsonDoc;

  jsonDoc["hum"] = 48;
  jsonDoc["temp"] = 25;
  jsonDoc["wet_i"] = int(4000);
  jsonDoc["light_i"] = 169;
  jsonDoc["dist"] = 0.25;
  jsonDoc["co2_ppm_i"] = 882;
  jsonDoc["pump_on"] = 0;

  String jsonPayload;
  serializeJson(jsonDoc, jsonPayload);

  // Send the HTTP POST request
  client.print(String("POST ") + urlPath + " HTTP/1.1\r\n" +
               "Host: " + serverIP + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " + jsonPayload.length() + "\r\n" +
               "Connection: close\r\n\r\n" +
               jsonPayload + "\r\n");

  // Close the connection
  client.stop();
}

void handleIncomingRequest() {
  if (server.method() == HTTP_POST) {
    String payload = server.arg("plain");
    Serial.println("Received POST request:");
    Serial.println(payload);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}
