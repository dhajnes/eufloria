#include <SoftwareSerial.h>
#include <ArduinoJson.h>

SoftwareSerial ard_2_node(11,12); // tx, rx
int n = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  ard_2_node.begin(9600);
  
  
}

void loop() {
  if (n > 100) {n = 0;}

  StaticJsonDocument<1000> doc;  // json6, new
//  JsonObject& data = jsonBuffer.createObject();  // json 5, old

  doc["number"] = n;
  doc["text"] = "Hi from Arduino!\n";

  serializeJson(doc, ard_2_node);
  
  delay(1000);

  n += 1;

}
