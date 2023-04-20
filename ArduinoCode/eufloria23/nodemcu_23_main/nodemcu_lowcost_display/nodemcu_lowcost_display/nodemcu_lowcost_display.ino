#include <U8x8lib.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <SoftwareSerial.h>
SoftwareSerial node_2_ard(D6,D5); // tx, rx

// Replace these with your own WiFi network credentials
const char* ssid = "Hotspot";
const char* password = "SPDPassword";

// The IP address and port number of the remote server
const char* serverIP = "10.42.0.1";
const int serverPort = 1880;
const char* urlPath = "/url";
volatile bool START_WATERING = false;
WiFiClient client;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);


void setup() {
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  Serial.begin(9600);
  node_2_ard.begin(19200);
//  WiFi.begin(ssid, password);
//
//  // setup the WIFI client
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//  }
  
}

struct Data{
  int temp = 99;
  int hum = 99;
  float dist = 999.9;
  int wet = 9999;
  int light = 9999;
  int co2 = 9999;
  bool pump = false;
  } data;

struct Data doc2data(struct Data data, StaticJsonDocument<128> &doc){
  data.temp = doc["temp"];
  data.hum = doc["hum"];
  data.dist = doc["dist"];
  data.wet = doc["wet"];
  data.light = doc["light"];
  data.co2 = doc["co2"];
  data.pump = doc["pump"];

  return data;
  }


void loop() {

  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, node_2_ard);

  if (error){
    Serial.println("Not received");
//    Serial.print(error);
    return;
  }
  else {
    Serial.println("Received json, drawing.");
    String jsonString;
//    serializeJson(doc, jsonString);
//    sendPostRequest(jsonString);
    data = doc2data(data, doc);
    Serial.print("Received for example: data.temp:");
    Serial.println(data.temp);
    Serial.print("Received for example: data.hum:");
    Serial.println(data.hum);
    Serial.print("Received for example: data.dist:");
    Serial.println(data.dist);
    Serial.print("Received for example: data.light:");
    Serial.println(data.light);
    
    
  }
drawData(data);
  
}


void sendPostRequest(String jsonPayload) {
  if (!client.connect(serverIP, serverPort)) {
    return;
  }

  // Send the HTTP POST request
  client.print(String("POST ") + urlPath + " HTTP/1.1\r\n" +
               "Host: " + serverIP + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " + jsonPayload.length() + "\r\n" +
               "Connection: close\r\n\r\n" +
               jsonPayload + "\r\n");

  // Listen for the server's response
  unsigned long timeout = millis();
  while (client.connected() && (millis() - timeout < 3000)) {
    if (client.available()) {
      String response = client.readStringUntil('\r');
      // Serial.println("Received response: " + response);
      
      if (response.indexOf("water") >= 0) {
        START_WATERING = true;
      }
      
      timeout = millis();
    }
  }

  // Close the connection
  client.stop();
}


void drawData(struct Data data){
  // clear display
  u8x8.clearDisplay();
  u8x8.setCursor(4,0);
  u8x8.print("| STATS |");
  
  // display temperature
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0,1);
  u8x8.print("T: ");
  u8x8.print(int(data.temp));
  u8x8.print(" C");
//
//  // display humidity
  u8x8.setCursor(7, 1);
  u8x8.print("| H: ");
  u8x8.print(int(data.hum));
  u8x8.print(" %"); 

//  // display moisture
  u8x8.setCursor(0, 2);
  u8x8.print("M: ");
  u8x8.print(data.wet);
//  u8x8.print(" -");

//  // display light
  u8x8.setCursor(7, 2);
  u8x8.print("| l: ");
  u8x8.print(data.light);
//  u8x8.print(" -");

  // display dist
  u8x8.setCursor(0, 3);
  u8x8.print("d: ");
  u8x8.print(70.05);
  u8x8.print(" cm");

  // display CO2
  u8x8.setCursor(0, 4);
  u8x8.print("CO2: ");
  u8x8.print(data.co2);
  u8x8.print(" ppm");

  // display pump status
  if (data.pump){
    u8x8.setCursor(0, 5);
    u8x8.print("Pmp: ON");  
  }
  else{
    u8x8.setCursor(0, 5);
    u8x8.print("Pmp: OFF");
  }

  u8x8.refreshDisplay();
}
