//#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>

#include <SoftwareSerial.h>
SoftwareSerial node_2_ard(D6,D5); // tx, rx

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   40
#define LOGO_WIDTH    24

// Replace these with your own WiFi network credentials
const char* ssid = "Hotspot";
const char* password = "SPDPassword";

// The IP address and port number of the remote server
const char* serverIP = "10.42.0.1";
const int serverPort = 1880;
const char* urlPath = "/url";
volatile bool START_WATERING = false;

WiFiClient client;
//ESP8266WebServer server(80);


static const unsigned char PROGMEM logo_bmp[] = { 
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x7, 
  0x0, 0x0, 0xf, 0x0, 0x0, 0x3f, 0x0, 0x0, 0xff, 0x0, 0x3, 0xff, 
  0x0, 0xf, 0xff, 0x0, 0x3f, 0xff, 0x0, 0x7f, 0xee, 0x1, 0xff, 0xee, 
  0x3, 0xff, 0xce, 0x7, 0xff, 0xde, 0x7, 0xff, 0x9c, 0xf, 0xff, 0x3c, 
  0xf, 0xfe, 0x3c, 0x1f, 0xfe, 0x78, 0x1f, 0xf8, 0xf8, 0x1f, 0xf1, 0xf0, 
  0x1f, 0xe3, 0xe0, 0x1f, 0xc7, 0xc0, 0x1f, 0x1f, 0x80, 0x1c, 0x3f, 0x0, 
  0x10, 0xfc, 0x0, 0x3, 0xf0, 0x0, 0xf, 0xc0, 0x0, 0x1f, 0x0, 0x0, 
  0x38, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
};

struct Data{
  float temp = 0;
  float hum = 0;
  float dist = 0;
  int wet = 0;
  int light = 0;
  int co2_ppm = 0;
  bool pump_on = false;
  } data;


struct Data doc2data(struct Data data, StaticJsonDocument<256> &doc){
  data.temp = doc["temp"];
  data.hum = doc["hum"];
  data.dist = doc["dist"];
  data.wet = doc["wet_i"];
  data.light = doc["light_i"];
  data.co2_ppm = doc["co2_ppm_i"];
  data.pump_on = doc["pump_on"];

  return data;
  }


void setup() {
  node_2_ard.begin(19200);

  WiFi.begin(ssid, password);

  // setup the WIFI client
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
//    Serial.print(".");
  }

  // setup the HTTP server
//  server.on("/esp", handleIncomingRequest);
//  server.begin();
//  Serial.println("HTTP server started");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
//    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  drawLogo();
  
}


void loop() {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, node_2_ard);

  //TODO write logic for when to send and when not
  
//  server.handleClient();
  // Send POST request to the remote server
  
  
  if (error){
//    Serial.println("Invalid JSON object");
//    Serial.println(error.c_str());
    return;
  }
  else {
    String jsonString;
    serializeJson(doc, jsonString);
    sendPostRequest(jsonString);
    data = doc2data(data, doc);
  }

//  if (START_WATERING){
//    StaticJsonDocument<256> doc;
//    doc["water"] = true;
//    unsigned long start_time = millis();
//    while (millis() - start_time < 5000) {
////      node_2_ard.println(json_str);
//      serializeJson(doc, node_2_ard);
//      delay(500);
//    }
//    START_WATERING = false;
//  }
 

  drawDHT(data);
//  delay(50);
}


void sendPostRequest(String jsonPayload) {
  if (!client.connect(serverIP, serverPort)) {
    // Serial.println("Connection failed");
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


void drawDHT(struct Data data){
  // clear display
  display.clearDisplay();
  display.setCursor(40,0);
  display.print("| STATS |");
  
  // display temperature
  display.setTextSize(1);
  display.setCursor(0,20);
  display.print("T: " + String(int(data.temp)) + " C");

  // display humidity
  display.setCursor(63, 20);
  display.print("| H: " + String(int(data.hum)) + " %"); 

  // display moisture
  display.setCursor(0, 30);
  display.print("M: " + String(data.wet) + " -");

  // display light
  display.setCursor(63, 30);
  display.print("| l: " + String(data.light) + " -");


  // display dist
  display.setCursor(0, 40);
  display.print("d: " + String(data.dist) + " cm");

  // display CO2
  display.setCursor(0, 50);
  display.print("CO2: " + String(data.co2_ppm) + " ppm");

  // display pump status
  display.setCursor(80, 40);
  if (data.pump_on){
    display.print("Pmp: ON");  
  }
  else{
    display.print("Pmp: OFF");
    }
  
  
  display.display(); 
}

void drawLogo(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2 - 10,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);

  display.setTextSize(1);  
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.setCursor(display.width()/2 - 25, 44);
  display.println(F("eufloria"));
  display.display();

  display.setCursor(display.width()/2 - 10, 54);
  display.println(F("v.23"));
  display.display();
  
  display.invertDisplay(true);
  display.display();
  delay(1000);
  display.invertDisplay(false);
  display.display();
  delay(1000);
  display.invertDisplay(true);
  display.display();
  delay(1000);
  display.invertDisplay(false);
  display.display();
  delay(1000);
}
