
// NODEMCU

#include <ArduinoJson.h>
#include <SoftwareSerial.h>

SoftwareSerial s(D7,D8); //RX, TX
int data;

void setup() {
  s.begin(9600);
  Serial.begin(9600);
  while (!Serial) continue;
}
 
void loop() {
  char buffer[512];
  DynamicJsonDocument doc(256);
  deserializeJson(doc, s);
  serializeJsonPretty(doc, buffer);
  
 
 
  //Serial.println("JSON received and parsed");
  Serial.println(buffer);
  //s.write("s");
//  if (s.available()>0)
//  {
//    data=s.read();
//    Serial.println(data);
//  }
 
 
}
