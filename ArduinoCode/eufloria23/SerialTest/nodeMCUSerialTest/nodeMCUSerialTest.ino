#include <SoftwareSerial.h>
#include <ArduinoJson.h>

SoftwareSerial node_2_ard(D5,D6); // tx, rx


void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  node_2_ard.begin(9600);
  
}

void loop(){
  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, node_2_ard);

  if (error){
    Serial.println("Invalid JSON object");
    return;
  }

  Serial.println("Read JSON correctly.");
  Serial.print("Number: ");
  int num = doc["number"];
  Serial.println(num);
  Serial.print("Text: ");
  String txt = doc["text"];
  Serial.println(txt);
  Serial.println("-------------");
}


//void loop() {
//
//  while(node_2_ard.available() > 0)
//  {
//    c = node_2_ard.read();
//    if (c=='\n'){
//      break;
//    }
//    else{
//      dataIn += c;
//    }
//  }
//  if (c == '\n'){
//    Serial.println(dataIn);
//    c = 0;
//    dataIn="";
//  }
//  
//  delay(100);
//
//}
