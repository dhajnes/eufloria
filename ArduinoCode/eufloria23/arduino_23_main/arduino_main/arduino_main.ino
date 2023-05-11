
#include "DHT.h"
#include <SPI.h>
// #include <SD.h>
#include <stdio.h>
#include <Wire.h>
// #include <SoftwareSerial.h>
#include <ArduinoJson.h>

// SoftwareSerial ard_2_node(12,11); // rx, tx
// SoftwareSerial ard_2_pc(0,1);

// DHT constants
#define DHTPIN 10
#define DHTTYPE DHT11

// CO2 sensor constants
#define CO2_INTERRUPT 2
#define ADRESS_CDM_7160 0x69
#define ADRESS_REGISTER 0x03

#define wetPin0 A0
#define lightPin A1
#define echoPin 8 // Ultrasound sensor constants
#define trigPin 9
#define pumpPin 3

#define BUFFER_SIZE 64
char buffer[BUFFER_SIZE];
int strBufferIndex = 0;

char reading_buffer[BUFFER_SIZE];
int reading_strBufferIndex = 0;

unsigned long previousMillis = 0;
const unsigned long SENDING_INTERVAL = 5000;


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

int ch2int(byte c)
{
    return int(c) - int('0');
}

// DHT 11 sensor variables
DHT dht(DHTPIN, DHTTYPE);
int hum = 0;
int temp = 0;

// CO2 sensor variables
byte co2_lower_bits = 0;
byte co2_upper_bits = 0;
int co2_ppm = 0;
volatile bool sensor_free = true;

const int AVG_BUFFER_SIZE = 5;
float running_average_wet = 0;
int bufferIndex = 0;
float run_avg_arr[AVG_BUFFER_SIZE] = {0};

// sensor variables
int wetVal0 = 0;
int lightVal = 0;
bool pump_on = false;
int send_pump_on_k_times = 5;

float get_run_avg(float new_meas)
{

    run_avg_arr[bufferIndex] = new_meas;
    bufferIndex = (bufferIndex + 1) % AVG_BUFFER_SIZE;

    float sum = 0;
    for (int i = 0; i < AVG_BUFFER_SIZE; i++)
    {
        sum += run_avg_arr[i];
    }

    float avg = sum / AVG_BUFFER_SIZE;
    return avg;
}

void sensor_interrupt()
{
    sensor_free = true;
}

float echo_distance()
{
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


void setup()
{
    // pinmodes
    pinMode(CO2_INTERRUPT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CO2_INTERRUPT), sensor_interrupt, FALLING);
    Wire.begin();

    pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
    pinMode(echoPin, INPUT);  // Sets the echoPin as an
    pinMode(pumpPin, OUTPUT);

    Serial.begin(19200);
    // Serial.begin(19200);
    //  ard_2_node.begin(19200);
    //  ard_2_pc.begin(9600);
    dht.begin();
}

void loop()
{

    delay(50);
    //  StaticJsonDocument<256> doc;
    //  StaticJsonDocument<256> get_doc;

    //  DeserializationError error = deserializeJson(get_doc, Serial);
    unsigned long currentMillis = millis();
    
    String msg = readMessageSerial();
    // Serial.println(msg);
    bool start_watering = false;

    if (msg.compareTo("-1") != 0)  // compareTo() returns index of identical substring until the end, so if returns 0 then identical
    {
        Serial.print("\n\n >>start_watering_msg: ");
        Serial.println(msg);
        start_watering = start_watering_msg(msg);
        
    }
    
    


    hum = dht.readHumidity();
    temp = dht.readTemperature();

    wetVal0 = analogRead(wetPin0);
    lightVal = analogRead(lightPin);

    float distance = echo_distance();

    if (isnan(hum) || isnan(temp))
    {
        //    ard_2_pc.println(F("Failed to read from DHT sensor!"));
        return;
    }

    running_average_wet = get_run_avg(float(wetVal0));

    // 750 seems to be wetness threshold for a week and a half unirrigated plant -> 750+ is dry 1024 is probably the driest, should equal 0
    // 450 - 600 freshly irrigated plant

    float WET_MAX = 1024.0;
    float WET_MIN = 450.0;

    if (wetVal0 > WET_MAX)
        wetVal0 = WET_MAX;
    if (wetVal0 < WET_MIN)
        wetVal0 = WET_MIN;

    // flip values and normalize the to <0,1> range
    int wet = (WET_MAX - WET_MIN - (wetVal0 - WET_MIN)) * 100 / (WET_MAX - WET_MIN);
    

    float LGH_MAX = 725.0;
    float LGH_MIN = 25.0;   

    // Serial.print("lightVal: ");
    // Serial.println(lightVal);

    if (lightVal > LGH_MAX)
        lightVal = LGH_MAX;
    if (lightVal < LGH_MIN)
        lightVal = LGH_MIN;

    int lgh = (LGH_MAX - LGH_MIN - (lightVal - LGH_MIN)) * 100 / (LGH_MAX - LGH_MIN);
    


    if (running_average_wet > 850.0 || start_watering)
    {   
        unsigned long startTime = millis();
        unsigned long timer1 = 1000;  // 1 second timer
        if (start_watering) {
            Serial.println("\n\n >> Watering because of manual input! <<\n\n");
            while (millis() - startTime < timer1) {
                pump_on = true;
                digitalWrite(pumpPin, HIGH);
                send_pump_on_k_times = 5;
            }
        }        
    }
    else
    {
        pump_on = false;
        digitalWrite(pumpPin, LOW);
    }

    if (sensor_free)
    {
        Wire.beginTransmission(ADRESS_CDM_7160); // verified with I2C scanner
        Wire.write(ADRESS_REGISTER);
        Wire.endTransmission();
        Wire.requestFrom(ADRESS_CDM_7160, 2); // request 2 bytes
        co2_lower_bits = Wire.read();
        co2_upper_bits = Wire.read();
        co2_ppm = (co2_upper_bits << 8) | co2_lower_bits; // lower + upper shifted by 8 = resulting ppm
        co2_ppm += 200;                                   // offeset
                                                          //    ard_2_pc.print(F("CO2 ppm: "));
                                                          //    ard_2_pc.println(co2_ppm);
    }
    
    if (currentMillis - previousMillis >= SENDING_INTERVAL) 
    {   
        if (send_pump_on_k_times > 0){
            pump_on = true;
            send_pump_on_k_times--;
        }
        data = Data();
        data.hum = hum;
        data.tmp = temp;
        data.wet = wet;
        data.lgh = lgh;
        data.dst = distance;
        data.co2 = co2_ppm;
        data.pmp = pump_on;
        // Update the previousMillis variable
        previousMillis = currentMillis;
        send_data_over_serial(data);
    }

}


void send_data_over_serial(Data &data)
{
    
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "*hum,tmp,wet,lgh,dst,co2,pmp;%03d,%03d,%03d,%03d,%04d,%04d,%dX",
            data.hum, data.tmp, data.wet, data.lgh, data.dst, data.co2, data.pmp);

    // compute and append a XOR checksum
    int k = 0;
    byte checksum = 0;
    while (buffer[k] != 'X' && k < BUFFER_SIZE)
    {
        checksum ^= buffer[k];
        k++;
    }

    // fill buffer also with the appended checksum
    snprintf(buffer, BUFFER_SIZE, "*hum,tmp,wet,lgh,dst,co2,pmp;%03d,%03d,%03d,%03d,%04d,%04d,%dX%03d",
            data.hum, data.tmp, data.wet, data.lgh, data.dst, data.co2, data.pmp, checksum);
    Serial.println(buffer);

    delay(500);
    // Serial.flush();
}


String readMessageSerial()
{   

    
    bool finished_reading = false;
    bool started_reading = false;
    unsigned long timeout = 5000;
    unsigned long read_start = millis();
    while (!finished_reading)
    {   
        if (millis() - read_start > timeout){
            Serial.println("Timeout on reading.");
            reading_strBufferIndex = 0;
            finished_reading = true;
            started_reading = false;
            sprintf(reading_buffer, "%d", -1);
            String str_buffer(reading_buffer);

            return str_buffer;
        }
        if (Serial.available() > 0)
        {   
            // Serial.println("AVAILABLE.");
            char incomingByte = Serial.read();
            // Serial.print("CHAR: ");
            // Serial.println(incomingByte);
            if (incomingByte == '#' || started_reading)
            {   
                // Serial.println("STARTED READING.");
                if (incomingByte == '#')
                {   
                    // Serial.print("BECAUSE INCOMING BYTE IS.");
                    // Serial.println(incomingByte);
                    reading_buffer[reading_strBufferIndex++] = incomingByte;
                    started_reading = true;
                    continue;
                }
                else if (incomingByte == '\n')
                {
                    // Null-terminate the reading_buffer
                    reading_buffer[reading_strBufferIndex] = '\0';

                    // Print the received string
                    Serial.println(reading_buffer);

                    // Reset the reading_buffer index
                    reading_strBufferIndex = 0;
                    finished_reading = true;
                    started_reading = false;
                    
                    // compute the XOR the checksum
                    byte checksum = 0;
                    int k = 0;
                    while (reading_buffer[k] != 'X' && k < BUFFER_SIZE)
                        {
                            // Serial.print("reading_buffer[k]: ");
                            // Serial.println(reading_buffer[k]);
                            checksum ^= reading_buffer[k];
                            k++;
                        }
                    // Serial.print("Computed checksum: ");
                    // Serial.println(checksum);
                    // Serial.println(b, HEX);
                    // Serial.println(checksum);
                    
                    
                    if (k+3 < BUFFER_SIZE)
                    {   Serial.println(ch2int(reading_buffer[k+1]));
                        Serial.println(ch2int(reading_buffer[k+2]));
                        Serial.println(ch2int(reading_buffer[k+3]));
                        int provided_checksum = 100 * ch2int(reading_buffer[k+1]) + 10 * ch2int(reading_buffer[k+2]) + ch2int(reading_buffer[k+3]);
                        
                        if (checksum != provided_checksum)          
                        {   
                            // if checksum incorrect, return "-1" string
                            // if checksum correct
                            Serial.print("Provided checksum: ");
                            Serial.println(provided_checksum);
                            Serial.print("incorrect checksum: ");
                            Serial.println(checksum);
                            sprintf(reading_buffer, "%d", -1);
                        }
                        
                    }
                    else
                    {   
                        // if checksum incorrect, return "-1" string
                        sprintf(reading_buffer, "%d", -1);
                    }
                    
                    String str_buffer(reading_buffer);

                    return str_buffer;
                }
                else
                {
                    // Add the incoming byte to the reading_buffer
                    reading_buffer[reading_strBufferIndex++] = incomingByte;

                    // Check for reading_buffer overflow
                    if (reading_strBufferIndex >= BUFFER_SIZE)
                    {
                        Serial.println("Buffer overflow!");
                        reading_strBufferIndex = 0;
                        finished_reading = true;
                        started_reading = false;
                        sprintf(reading_buffer, "%d", -1);
                        String str_buffer(reading_buffer);

                        return str_buffer;
                    }
                }
            }
            
            if (!started_reading) break;
        }
        // finished_reading = true;
    }

    sprintf(reading_buffer, "%d", -1);
    String str_buffer(reading_buffer);

    return str_buffer;
}

bool start_watering_msg(String message)
{   
    Serial.print("Got start_watering_msg: ");
    Serial.println(message);
    bool start_watering = 0;

    // int index = 0;
    // if (index > 0)
    // {
    //     message = message.substring(index+1, message.length());
    // }
    int start = 1;  // skipping the #
    int end = message.indexOf("X");
    
    if (end > start)
    {
        start_watering = message.substring(start, end).toInt();
    }
    
    return start_watering;
}
