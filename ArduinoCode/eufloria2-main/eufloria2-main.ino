#include "DHT.h"
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>

// DHT thermo and hygrometer setup
#define DHTPIN 10
#define DHTTYPE DHT11

// setup of the soil moisture sensors
#define wetPin0 A0
#define wetPin1 A1
#define wetPin2 A2
#define wetPin3 A3

// setup of the ultrasound sensor for the canister water level
#define echoPin 8 
#define trigPin 9 

#define pumpPin 3
#define valvePin1 2
#define valvePin2 1
#define valvePin3 0


// setup of the solar panel
#define lightPin4 A4

#define chipSelect 4

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial s(5,6); //RX, TX

int wetVal0 = 0;
int wetVal1 = 0;
int wetVal2 = 0;
int wetVal3 = 0;

float hum = 0;
float temp = 0;

int lightVal = 0;

struct Time {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  } timestamp;

const float tankHeight = 36;  //cm
const float tankCapacity = 36;  //litres - a coincidence :D
float tankState = 0; //litres
DynamicJsonDocument doc(256);

void setup() {
  pinMode(pumpPin, OUTPUT);
  pinMode(valvePin1, OUTPUT);
  pinMode(valvePin2, OUTPUT);
  pinMode(valvePin3, OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  //Serial.begin(9600);
  s.begin(4800);
  dht.begin();
  
}

Time getInternetTime(){
  // send a serial comm reqeust for time
  // for example a "t"
  if(s.available()>0){
    //s.write(data);

    //doc.printTo(s);
    serializeJsonPretty(doc, s);
  }
  
  }

float checkWaterReserve(){
  // Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  float duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  float distance = (duration * 0.034) / 2; // Speed of sound wave divided by 2 (go and back)
  
  // 38 cm is the active height of the canister - 4 cm of sensor overhead = 34
  // the volume shall then be cca 3.4 * 3.6 * 2.8 = ~ 34 litres
  // current volume of water in the tank: (height - measValue)/height * fullVolume
  return (tankHeight - distance)/tankHeight * tankCapacity;  //litres
  }

void saveToCard(){
  
  }




void loop() {

delay(1000);

tankState = checkWaterReserve();
lightVal = analogRead(lightPin4);
hum = dht.readHumidity();
temp = dht.readTemperature();
//Serial.print(lightValue);
//Serial.println();

//Serial.print(F("Humidity: "));
//Serial.print(hum);
//Serial.print(F("%  Temperature: "));
//Serial.print(temp);
//Serial.print(F("°C "));

DynamicJsonDocument doc(256);

JsonObject moist = doc.createNestedObject("moist");
JsonObject timestamp = doc.createNestedObject("timestamp");
moist["moist0"] =wetVal0;
moist["moist1"] =wetVal1;
moist["moist2"] =wetVal2;
moist["moist3"] =wetVal3;

doc["light"] = lightVal;
doc["temp"] = temp;
doc["hum"] = hum;
doc["tankstate"] = tankState;
timestamp["date"] = 210712;  //TODO time by NodeMCU
timestamp["time"] = 151927;  //TODO time by NodeMCU

if(s.available()>0){
    //s.write(data);

    //doc.printTo(s);
    serializeJsonPretty(doc, s);
  }
  


}
