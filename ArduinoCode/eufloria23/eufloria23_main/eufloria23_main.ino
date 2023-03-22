
#include "DHT.h"
#include <SPI.h>
//#include <SD.h>
#include <stdio.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

SoftwareSerial ard_2_node(12,11); // tx, rx


// DHT constants 
#define DHTPIN 10
#define DHTTYPE DHT11

// CO2 sensor constants
#define CO2_INTERRUPT 2
#define ADRESS_CDM_7160 0x69
#define ADRESS_REGISTER 0x03

// Moisture sensor constants
#define wetPin0 A0


// Light sensor constants
#define lightPin A1

// Ultrasound sensor constants
#define echoPin 8 
#define trigPin 9 

// Pump sensor constants
#define pumpPin 3


// DHT 11 sensor variables
DHT dht(DHTPIN, DHTTYPE);
float hum = 0;
float temp = 0;

// CO2 sensor variables
byte co2_lower_bits = 0;
byte co2_upper_bits = 0;
int co2_ppm = 0;
volatile bool sensor_free = true;

const int AVG_BUFFER_SIZE = 5;
float running_average_wet = 0;
int bufferIndex = 0;
float run_avg_arr[AVG_BUFFER_SIZE] = {0};

// Moisture sensor variables
int wetVal0 = 0;

// Light sensor variables
int lightVal = 0;

// Pump variables
bool pump_on = false;

float get_run_avg(float new_meas){

  run_avg_arr[bufferIndex] = new_meas;
  bufferIndex = (bufferIndex + 1) % AVG_BUFFER_SIZE;

  float sum = 0;
  for (int i = 0; i < AVG_BUFFER_SIZE; i++){
    sum += run_avg_arr[i];
  }
  
  float avg = sum / AVG_BUFFER_SIZE;
  return avg;
}


void sensor_interrupt() {
  sensor_free = true;  
}


float echo_distance(){
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
  
  return distance;
}







void setup() {
  // pinmodes
  pinMode(CO2_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CO2_INTERRUPT), sensor_interrupt, FALLING );
  Wire.begin();

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an 
  pinMode(pumpPin, OUTPUT);
  
//  Serial.begin(19200);
  ard_2_node.begin(9600);
  dht.begin();
}


void loop() {
  
  delay(1000);
  StaticJsonDocument<256> doc;
  
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  
  wetVal0 = analogRead(wetPin0);
  lightVal = analogRead(lightPin);

  float distance = echo_distance();

  if (isnan(hum) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  running_average_wet = get_run_avg(float(wetVal0));

  if (running_average_wet > 850.0){
    pump_on = true;
    digitalWrite(pumpPin, HIGH);
  }
  else {
    pump_on = false;
    digitalWrite(pumpPin, LOW);
  }

  
//  Serial.print(F("Humidity: "));
//  Serial.print(hum);
//  Serial.print(F("%  Temperature: "));
//  Serial.print(temp);
//  Serial.print(F("°C "));
//  Serial.print("Moisture reading: ");
//  Serial.print(wetVal0);
//  Serial.print(" Light value: ");
//  Serial.print(lightVal);
//  Serial.print(" Distance: ");
//  Serial.println(distance);
  

  if (sensor_free) {
    Wire.beginTransmission(ADRESS_CDM_7160); // verified with I2C scanner
    Wire.write(ADRESS_REGISTER); 
    Wire.endTransmission();
    Wire.requestFrom(ADRESS_CDM_7160, 2); // request 2 bytes
    co2_lower_bits = Wire.read(); 
    co2_upper_bits = Wire.read(); 
    co2_ppm = (co2_upper_bits << 8) | co2_lower_bits; //lower + upper shifted by 8 = resulting ppm
    co2_ppm += 200;  // offeset
    Serial.print(F("CO2 ppm: "));
    Serial.println(co2_ppm); 
  }

  

  doc["hum"] = hum;
  doc["temp"] = temp;
  doc["wet_i"] = int(running_average_wet);
  doc["light_i"] = lightVal;
  doc["dist"] = distance;
  doc["co2_ppm_i"] = co2_ppm;
  doc["pump_on"] = pump_on;

  serializeJson(doc, ard_2_node);

}
