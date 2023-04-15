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
  StaticJsonDocument<128> jsonDoc;
  JsonArray tempArray = jsonDoc.createNestedArray("temp");
  tempArray.add(0);
  tempArray.add(2);
  tempArray.add(2);

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


//
//// Replace these with your own WiFi network credentials
//const char* ssid = "Hotspot";
//const char* password = "SPDPassword";
//
//// The IP address and port number of the server
//const char* serverIP = "10.42.0.1";
//const int serverPort = 1880;
//const char* urlPath = "/url";
//
//WiFiClient client;
//
//void setup() {
//  Serial.begin(115200);
//  delay(10);
//
//  // Connect to Wi-Fi
//  Serial.println();
//  Serial.print("Connecting to ");
//  Serial.println(ssid);
//
//  WiFi.begin(ssid, password);
//
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//
//  Serial.println("");
//  Serial.println("WiFi connected");
//  Serial.println("IP address: ");
//  Serial.println(WiFi.localIP());
//}
//
//void loop() {
//  if (!client.connect(serverIP, serverPort)) {
//    Serial.println("Connection failed");
//    delay(5000);
//    return;
//  }
//
//  // Prepare JSON payload
//  StaticJsonDocument<128> jsonDoc;
//  JsonArray tempArray = jsonDoc.createNestedArray("temp");
//  tempArray.add(0);
//  tempArray.add(2);
//  tempArray.add(2);
//
//  String jsonPayload;
//  serializeJson(jsonDoc, jsonPayload);
//
//  // Send the HTTP POST request
//  client.print(String("POST ") + urlPath + " HTTP/1.1\r\n" +
//               "Host: " + serverIP + "\r\n" +
//               "Content-Type: application/json\r\n" +
//               "Content-Length: " + jsonPayload.length() + "\r\n" +
//               "Connection: close\r\n\r\n" +
//               jsonPayload + "\r\n");
//
//  // Wait for the server's response
//  unsigned long timeout = millis();
//  while (client.available() == 0) {
//    if (millis() - timeout > 5000) {
//      Serial.println(">>> Client Timeout!");
//      client.stop();
//      delay(5000);
//      return;
//    }
//  }
//
//  // Print the server's response
//  while (client.available()) {
//    String line = client.readStringUntil('\r');
//    Serial.print(line);
//  }
//
//  // Close the connection
//  client.stop();
//
//  // Wait for 30 seconds before sending the next request
//  delay(30000);
//}


//// Replace these with your own WiFi network credentials
//const char* ssid = "Hotspot";
//const char* password = "SPDPassword";
//
//// The IP address and port number of the server
//const char* serverIP = "10.42.0.1";
//const int serverPort = 1880;
//const char* urlPath = "/url";
//
//WiFiClient client;
//
//void setup() {
//  Serial.begin(115200);
//  delay(10);
//
//  // Connect to Wi-Fi
//  Serial.println();
//  Serial.print("Connecting to ");
//  Serial.println(ssid);
//
//  WiFi.begin(ssid, password);
//
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//
//  Serial.println("");
//  Serial.println("WiFi connected");
//  Serial.println("IP address: ");
//  Serial.println(WiFi.localIP());
//}
//
//void loop() {
//  if (!client.connect(serverIP, serverPort)) {
//    Serial.println("Connection failed");
//    delay(5000);
//    return;
//  }
//
//  // Send the HTTP GET request
//  client.print(String("GET ") + urlPath + " HTTP/1.1\r\n" +
//               "Host: " + serverIP + "\r\n" +
//               "Connection: close\r\n\r\n");
//
//  // Wait for the server's response
//  unsigned long timeout = millis();
//  while (client.available() == 0) {
//    if (millis() - timeout > 5000) {
//      Serial.println(">>> Client Timeout!");
//      client.stop();
//      delay(5000);
//      return;
//    }
//  }
//
//  // Print the server's response
//  while (client.available()) {
//    String line = client.readStringUntil('\r');
//    Serial.print(line);
//  }
//
//  // Close the connection
//  client.stop();
//
//  // Wait for 30 seconds before sending the next request
//  delay(5000);
//}

//
//int httpCode;
//HTTPClient http;
//
//void setup() {
//  Serial.begin(115200);
//
//  // Initialize the WiFi connection
//  WiFi.begin("Hotspot", "SPDPassword");
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(1000);
//    Serial.println("Connecting to WiFi...");
//  }
//  Serial.println("Connected to WiFi!");
//
//  // Make a GET request to the specified URL
//  // http.begin("http://your_server_ip_address:your_server_port/your_endpoint");
//  begin(http, "10.42.0.1:1880/url");
//   
//}
//
//void loop() {
//  // Print the response code and body
//  httpCode = http.GET();
//  if (httpCode > 0) {
//    Serial.printf("Response code: %d\n", httpCode);
//    String response = http.getString();
//    Serial.println("Response body:");
//    Serial.println(response);
//  } else {
//    Serial.println("Error making HTTP request");
//  }
//
//  // Close the HTTP connection
////  http.end();
//}
