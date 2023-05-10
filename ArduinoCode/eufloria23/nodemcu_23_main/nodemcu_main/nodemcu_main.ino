
// language=Arduino
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <U8x8lib.h>


#define BUFFER_SIZE 64
// Arduino
char buffer[BUFFER_SIZE];
int bufferIndex = 0;

const char* ssid = "wutangwlan";
const char* password = "hahahachichichi";

// const char* ssid = "Hotspot";
// const char* password = "SPDPassword";

// The IP address and port number of the remote server
const char* serverIP = "10.42.0.1";
const int serverPort = 1880;
const char* urlPath = "/url";

volatile bool START_WATERING = false;
const int REFRESH_PERIOD = 1000;  //
unsigned long REFRESH_TIMEOUT = millis();
unsigned long FAKE_CALL_TIMER = millis();

bool ONCE_TRIGGER = false;

WiFiClient client;
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE);

struct Data
{
    int tmp = 0;
    int hum = 0;
    int dst = 0;
    int wet = 0;
    int lgh = 0;
    int co2 = 0;
    bool pmp = false;
} data;

// *hum,tmp,wet,lgh,dst,co2,pmp\r045,022,8264,1000,1000,0485,0\r0\n

int ch2int(byte c)
{
    return int(c) - int('0');
}

struct Data parseData(String message, Data data)
{   
    // Serial.print("[DEBUG] message: ");
    // Serial.println(message);

    int index = message.indexOf(';');
    if (index > 0)
    {
        message = message.substring(index+1, message.length());   
    }
    int start = 0;
    int end = message.indexOf(",");
    if (end > start)
    {
        data.hum = message.substring(start, end).toInt();
    }
    start = end + 1;
    end = message.indexOf(",", start);
    if (end > start)
    {
        data.tmp = message.substring(start, end).toInt();
    }
    start = end + 1;
    end = message.indexOf(",", start);
    if (end > start)
    {
        data.wet = message.substring(start, end).toInt();
    }
    start = end + 1;
    end = message.indexOf(",", start);
    if (end > start)
    {
        data.lgh = message.substring(start, end).toInt();
    }
    start = end + 1;
    end = message.indexOf(",", start);
    if (end > start)
    {
        data.dst = message.substring(start, end).toInt();
    }
    start = end + 1;
    end = message.indexOf(",", start);
    if (end > start)
    {
        data.co2 = message.substring(start, end).toInt();
    }
    start = end + 1;
    end = message.indexOf("X", start);
    if (end > start)
    {
        data.pmp = (message.substring(start, end).toInt() == 1);
    }
    return data;
}


String readMessageSerial()
{
    bool finished_reading = false;
    bool started_reading = false;
    while (!finished_reading)
    {
        if (Serial.available() > 0)
        {
            char incomingByte = Serial.read();
            if (incomingByte == '*' || started_reading)
            {   
                if (incomingByte == '*')
                {   
                    buffer[bufferIndex++] = incomingByte;
                    started_reading = true;
                    continue;
                }
                else if (incomingByte == '\n')
                {
                    // Null-terminate the buffer
                    buffer[bufferIndex] = '\0';

                    // Print the received string
                    Serial.println(buffer);

                    // Reset the buffer index
                    bufferIndex = 0;
                    finished_reading = true;
                    started_reading = false;
                    
                    // compute the XOR the checksum
                    byte checksum = 0;
                    int k = 0;
                    while (buffer[k] != 'X' && k < BUFFER_SIZE)
                        {   
                            checksum ^= buffer[k];
                            k++;
                        }
                    // Serial.print("Computed checksum: ");
                    // Serial.println(checksum);
                    // Serial.println(b, HEX);
                    // Serial.println(checksum);
                    
                    
                    if (k+3 < BUFFER_SIZE)
                    {   
                        int provided_checksum = 100 * ch2int(buffer[k+1]) + 10 * ch2int(buffer[k+2]) + ch2int(buffer[k+3]);
                        
                        if (checksum != provided_checksum)          
                        {   
                            // if checksum incorrect, return "-1" string
                            // if checksum correct
                            Serial.print("Provided checksum: ");
                            Serial.println(provided_checksum);
                            Serial.print("incorrect checksum: ");
                            Serial.println(checksum);
                            sprintf(buffer, "%d", -1);
                        }
                        
                    }
                    else
                    {   
                        // if checksum incorrect, return "-1" string
                        sprintf(buffer, "%d", -1);
                    }
                    
                    String str_buffer(buffer);

                    return str_buffer;
                }
                else
                {
                    // Add the incoming byte to the buffer
                    buffer[bufferIndex++] = incomingByte;

                    // Check for buffer overflow
                    if (bufferIndex >= BUFFER_SIZE)
                    {
                        Serial.println("Buffer overflow!");
                        bufferIndex = 0;
                        finished_reading = true;
                        started_reading = false;
                        sprintf(buffer, "%d", -1);
                        String str_buffer(buffer);

                        return str_buffer;
                    }
                }
            }
            
            if (!started_reading) break;
        }
    }

    sprintf(buffer, "%d", -1);
    String str_buffer(buffer);

    return str_buffer;
}


void setup()
{   
    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);

    Serial.begin(19200);
    WiFi.begin(ssid, password);
    
    // setup the WIFI client
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
}


void loop()
{
    
    
    
    String msg = readMessageSerial();
    if (msg == "-1")
    {
        // Serial.println("Invalid message.");
    }
    else
    {
        data = Data();
        data = parseData(msg, data);
        drawData(data);

        if ( millis() - REFRESH_TIMEOUT > REFRESH_PERIOD){
            REFRESH_TIMEOUT = millis();
            char json_like_msg[128];
            sprintf(json_like_msg, "{\"%s\":%d, \"%s\":%d, \"%s\":%d, \"%s\":%d, \"%s\":%d, \"%s\":%d, \"%s\":%d}", "hum", data.hum, "tmp", data.tmp, "wet", data.wet, "lgh", data.lgh, "dst", data.dst, "co2", data.co2, "pmp", data.pmp);
            String str(json_like_msg);
            // Serial.println(json_like_msg);
            sendPostRequest(json_like_msg);
            if (START_WATERING){
                
                // send signal to arduino to start watering
                Serial.println("\n\n>> SENDING DATA TO ARDUINO THAT NODEMCU RECEIVED MANUAL WATERING! <<\n\n");
                for (int i = 0; i < 5; i++){
                    char send_buffer[BUFFER_SIZE];
                    snprintf(send_buffer, BUFFER_SIZE, "#%dX", START_WATERING);
                    int k = 0;
                    byte checksum = 0;
                    while (send_buffer[k] != 'X' && k < BUFFER_SIZE)
                    {
                        checksum ^= send_buffer[k];
                        k++;
                    }
                    snprintf(send_buffer, BUFFER_SIZE, "#%dX%03d", START_WATERING, checksum);
                    
                    Serial.println(send_buffer);
                    // delay(100);
                    // Serial.print("Computed checksum: ");
                    // Serial.println(checksum);
                    delay(500);
                }
            
            }
         
        }   

        if (data.pmp == 1){
            Serial.println("\n\n >> RECEIVED DATA THAT PUMP IS ON, SETTING START_WATERING = FALSE! << \n\n");
            START_WATERING = false;
        } 
    }

    if (millis() - FAKE_CALL_TIMER > 7000){
        if (ONCE_TRIGGER == false){
            ONCE_TRIGGER = true;
            START_WATERING = true;
        }
        FAKE_CALL_TIMER = millis();
    }

    

}

void sendPostRequest(String payload)
{
    //  Serial.println("Sending Post Request...");
    if (!client.connect(serverIP, serverPort))
    {
           Serial.print("Couldn't connect to server with serverIP: ");
           Serial.println(serverIP);
           return;
    }

    //  Serial.println("Connected to server, sending HTTP request.");
    // Send the HTTP POST request
    client.print(String("POST ") + urlPath + " HTTP/1.1\r\n" +
                 "Host: " + serverIP + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + payload.length() + "\r\n" +
                 "Connection: close\r\n\r\n" +
                 payload + "\r\n");
    Serial.print("Able to send the POST request to: ");
    Serial.println(serverIP);
    Serial.print("With URL path: ");
    Serial.println(urlPath);


    // // Listen for the server's response
    //  unsigned long timeout = millis();
    // int n_of_trials = 50;
    // // int k = 0;
    // while (client.connected() && (millis() - timeout < 500))
    // { //&& (millis() - timeout < 500)) {
    //     if (client.available())
    //     {
    //         // String response = client.readStringUntil('\r');
    //         String response = client.readStringUntil('\n');
    //         // k++;
    //         Serial.print("Received response from server: ");
    //         Serial.println(response);
    //         if (response.indexOf("yes") >= 0)
    //         {
    //             START_WATERING = true;
    //             Serial.print("\n\n >> START_WATERING: ");
    //             Serial.println(START_WATERING);
    //         }
    //     }
    // }
    // client.stop();

    unsigned long timeout = millis();
    bool headersParsed = false;
    String payloadData = "";

    while (client.connected() && (millis() - timeout < 500))
    {
        if (client.available())
        {
            if (!headersParsed)
            {
                String line = client.readStringUntil('\n');
                if (line == "\r")
                {
                    headersParsed = true;
                }
            }
            else
            {
                payloadData += client.readStringUntil('\n');
            }
        }
    }

    client.stop();

    if (payloadData.length() > 0)
    {
        Serial.print("Received payload from server: ");
        Serial.println(payloadData);

        if (payloadData.indexOf("\"water\": \"yes\"") >= 0)
        {
            START_WATERING = true;
            Serial.print("\n\n >> START_WATERING: ");
            Serial.println(START_WATERING);
        }
    }
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
    u8x8.print(int(data.tmp));
    u8x8.print(" C");
    
    //  // display humidity
    u8x8.setCursor(7, 1);
    u8x8.print("| H: ");
    u8x8.print(int(data.hum));
    u8x8.print(" %");

    //  // display moisture
    u8x8.setCursor(0, 2);
    u8x8.print("M: ");
    u8x8.print(data.wet);
     u8x8.print(" %");

    //  // display light
    u8x8.setCursor(7, 2);
    u8x8.print("| l: ");
    u8x8.print(data.lgh);
     u8x8.print(" %");

    // display dist
    u8x8.setCursor(0, 3);
    u8x8.print("d: ");
    u8x8.print(data.dst);
    u8x8.print(" cm");

    // display CO2
    u8x8.setCursor(0, 4);
    u8x8.print("CO2: ");
    u8x8.print(data.co2);
    u8x8.print(" ppm");

    // display pump status
    if (data.pmp)
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
