#include <ArduinoJson.h>
// #include <SoftwareSerial.h>
// SoftwareSerial node_2_ard(D6,D5); // tx, rx
#include <U8x8lib.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
const char *ssid = "wutangwlan";
const char *password = "hahahachichichi";

// The IP address and port number of the remote server
const char *serverIP = "10.42.0.1";
const int serverPort = 1880;
const char *urlPath = "/url";

volatile bool START_WATERING = false;
WiFiClient client;
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE);

StaticJsonDocument<256> doc;

void setup()
{
    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);

    Serial.begin(9600);
    //  node_2_ard.begin(19200);

    WiFi.begin(ssid, password);

    // setup the WIFI client
    while (WiFi.status() != WL_CONNECTED)
    {
        //    Serial.print(".");
        delay(500);
    }
    //  Serial.println("");
}

struct Data
{
    int temp = 99;
    int hum = 99;
    float dist = 999.9;
    int wet = 9999;
    int light = 9999;
    int co2 = 9999;
    bool pump = false;
} data;

struct Data doc2data(struct Data data, StaticJsonDocument<256> doc)
{
    data.temp = doc["temp"];
    data.hum = doc["hum"];
    data.dist = doc["dist"];
    data.wet = doc["wet"];
    data.light = doc["light"];
    data.co2 = doc["co2"];
    data.pump = doc["pump"];

    return data;
}

void loop()
{

    data = Data();

    DeserializationError error = deserializeJson(doc, Serial);

    if (error)
    {
        //    Serial.println("Not received.");
        //    Serial.print(error);
        u8x8.clearDisplay();
        u8x8.setCursor(4, 0);
        u8x8.print("| Incorrect Json |");
        u8x8.refreshDisplay();
        return;
    }
    else
    {
        data = doc2data(data, doc);
        //    Serial.println("Received json, writing out.");
        //
        //    Serial.print("Received for example: data.temp:");
        //    Serial.println(data.temp);
        //    Serial.print("Received for example: data.hum:");
        //    Serial.println(data.hum);
        //    Serial.print("Received for example: data.dist:");
        //    Serial.println(data.dist);
        String jsonString;
        serializeJson(doc, jsonString);
        sendPostRequest(jsonString);
    }
    drawData(data);
}

void sendPostRequest(String payload)
{
    //  Serial.println("Sending Post Request...");
    if (!client.connect(serverIP, serverPort))
    {
        //    Serial.print("Couldn't connect to server with serverIP: ");
        //    Serial.println(serverIP);
    }

    //  Serial.println("Connected to server, sending HTTP request.");
    // Send the HTTP POST request
    client.print(String("POST ") + urlPath + " HTTP/1.1\r\n" +
                 "Host: " + serverIP + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + payload.length() + "\r\n" +
                 "Connection: close\r\n\r\n" +
                 payload + "\r\n");

    // Listen for the server's response
    //  unsigned long timeout = millis();
    int n_of_trials = 5;
    int k = 0;
    while (client.connected() && k <= n_of_trials)
    { //&& (millis() - timeout < 500)) {
        if (client.available())
        {
            String response = client.readStringUntil('\r');
            k++;
            if (response.indexOf("water") >= 0)
            {
                START_WATERING = true;
            }
        }
    }
    client.stop();
}

void drawData(struct Data data)
{
    // clear display
    u8x8.clearDisplay();
    u8x8.setCursor(4, 0);
    u8x8.print("| STATS |");

    // display temperature
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 1);
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
    u8x8.print(data.dist);
    u8x8.print(" cm");

    // display CO2
    u8x8.setCursor(0, 4);
    u8x8.print("CO2: ");
    u8x8.print(data.co2);
    u8x8.print(" ppm");

    // display pump status
    if (data.pump)
    {
        u8x8.setCursor(0, 5);
        u8x8.print("Pmp: ON");
    }
    else
    {
        u8x8.setCursor(0, 5);
        u8x8.print("Pmp: OFF");
    }

    u8x8.refreshDisplay();
}
