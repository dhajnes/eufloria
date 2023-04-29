
// language=Arduino
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define BUFFER_SIZE 64
// Arduino
char buffer[BUFFER_SIZE];
int bufferIndex = 0;

const char* ssid = "wutangwlan";
const char* password = "hahahachichichi";

// The IP address and port number of the remote server
const char* serverIP = "10.42.0.1";
const int serverPort = 1880;
const char* urlPath = "/url";

volatile bool START_WATERING = false;
WiFiClient client;

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
    Serial.println(message);

    int index = message.indexOf(';');
    if (index > 0)
    {
        // Serial.print("index of `;`: ");
        // Serial.println(index);
        message = message.substring(index+1, message.length());
        // Serial.print("Therefore message looks like: ");
        // Serial.println(message);
    }
    int start = 0;
    int end = message.indexOf(",");
    if (end > start)
    {
        data.hum = message.substring(start, end).toInt();
        Serial.print("hum: ");
        Serial.println(data.hum);
    }
    start = end + 1;
    end = message.indexOf(",", start);
    if (end > start)
    {
        data.tmp = message.substring(start, end).toInt();
        Serial.print("temp: ");
        Serial.println(data.tmp);
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
    Serial.begin(9600);
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
        sendPostRequest(msg);

    }

}

void sendPostRequest(String payload)
{
    //  Serial.println("Sending Post Request...");
    if (!client.connect(serverIP, serverPort))
    {
           Serial.print("Couldn't connect to server with serverIP: ");
           Serial.println(serverIP);
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

