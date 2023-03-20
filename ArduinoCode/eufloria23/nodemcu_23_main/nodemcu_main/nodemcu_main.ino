#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#include <SoftwareSerial.h>
SoftwareSerial node_2_ard(D6,D5); // tx, rx

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   40
#define LOGO_WIDTH    24
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

void setup() {
//  Serial.begin(9600);
  node_2_ard.begin(9600);

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

//  if (error){
//    Serial.println("Invalid JSON object");
//    Serial.println(error.c_str());
//    return;
//  }

  float temp = doc["temp"];
  float hum = doc["hum"];
  float dist = doc["dist"];
  int wet = doc["wet_i"];
  int light = doc["light_i"];
  int co2_ppm = doc["co2_ppm_i"];
  
  
//  Serial.println("Read JSON correctly.");
//  Serial.print("temp: ");
//  Serial.println(temp);
//  Serial.print("hum: ");
//  Serial.println(hum);
//  Serial.print("wet: ");
//  Serial.println(wet);
//  Serial.print("dist: ");
//  Serial.println(dist);
//  Serial.print("light: ");
//  Serial.println(light);
//  Serial.print("co2 ppm: ");
//  Serial.println(co2_ppm);
//  Serial.println("-------------");

  drawDHT(temp, hum);
  
}

void drawDHT(float temp, float hum){
  // clear display
  display.clearDisplay();
  
  // display temperature
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0,10);
  display.print(temp);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");
  
  // display humidity
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("Humidity: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(hum);
  display.print(" %"); 
  
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
