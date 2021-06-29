#include "mbed.h"
#include "USBSerial.h"
#include "Dht11.h"

Dht11 sensor(PA_6);
DigitalOut Buzzer(PA_0);
DigitalOut R(PA_2);
DigitalOut G(PA_3);
DigitalOut B(PA_4);
DigitalOut flowOn(PA_1);
AnalogIn ain1(PA_5);
AnalogIn lght(PA_7);

Ticker writer;

 struct GeneralPlant{
    // humidity
    int minHum;
    int maxHum;
    // temperature
    int minCels;
    int maxCels;
    // amount of light
    float minLight;
    float maxLight;
    // moisture
    int minMoist;
    int maxMoist;
    };
    
    struct Weights{
        float m;
        float l;
        float c;
        float h;
        };
        
int data_indx = 0;
const int sample_length = 100;
int data_temp[sample_length];
int data_moist[sample_length];
int data_light[sample_length];
int data_RH[sample_length];

//unsigned int moist =0;
int moist = 0;
//float light = 0.0f;
int light = 0;
int cels = 0;
int hum = 0;
unsigned char comIN =0;
float homeo = 1.0f; // should range from min 0.0 to max 1.0
int badness = 0;
int temp_crit;
int light_crit;
int moist_crit;
bool catching = false;

// for ANSII drawing
int h_length;
int h_magnitude;
int red_bnd;
int yel_bnd;


int buzz(float seconds, int freq){ 
    //float freq = 500; //500 Hz
    float half_period = (1.0/freq)/2;
    int length = (int)(seconds * freq);
    
    for (int b = 0; b<length; b++){
        wait(half_period);
        Buzzer = 1;
        wait(half_period);
        Buzzer = 0;
        }
        wait(0.1);
    for (int b = 0; b<length; b++){
        wait(half_period);
        Buzzer = 1;
        wait(half_period);
        Buzzer = 0;
        }
    return 0;
    }
    

int SosSignal(int Rin, int Gin, int Bin, int badness){
    float half_T;
    switch(badness){
        case 1:
            half_T = 0.25;
            break;
        case 2:
            half_T = 0.15;
            break;
        case 3:
            half_T = 0.1;
            break;
        default:
            half_T = 0;        
    }
    if(half_T == 0){
        return 1;
    }
    else{
        for (int i = 0; i < 2; i++){
            R = 0;
            G = 0;
            B = 0;
            wait(half_T);
            R = Rin;
            G = Gin;
            B = Bin;
            wait(half_T);
            if(badness == 2){
                buzz(0.5-half_T, 500);    
            }
            if(badness == 3){
                buzz(0.1, 700);
                buzz(0.1, 500);    
            }
            
        }
    }
    return 0;
}

void write(){
    data_RH[data_indx] = hum;
    data_light[data_indx] = light;
    data_moist[data_indx] = moist;
    data_temp[data_indx] = cels;
    
    
    data_indx++;
    }

int main() {    
    SYSCFG->CFGR1 |=0x10;   //Pins PA11/12 instead of pins PA9/10 (USB)
    USBSerial serial;
    
    serial.printf("MBED virtual serial port!\r\n");
    //int data_exa2[] = {1,2,3,4,5,6,7,8,9,10};
    
    GeneralPlant plant = {
    // humidity
    .minHum = 45, //minHum
    .maxHum = 75, //maxHum
    // temperature
    .minCels = 19, //minTemp
    .maxCels = 30, //maxTemp
    // light
    .minLight = 20, //minLight - shade
    .maxLight = 70, //maxLight - 2m from brightly lit window
    // moisture (CAREFUL! THE VALUES GROW INVERSLY! MAX -> small number
    .minMoist = 47000, // minMoisture
    .maxMoist = 26300 //maxMoisture
    };
    
    Weights weights = {
        // says how plant cares about attribute
        .m = 0.5, //weight of moist
        .l = 0.3, //we. of light
        .c = 0.15, //we. of temp in Cels
        .h = 0.05 //we. of hum
    };
    
    for (int b = 0; b<100; b++){
        wait(0.001);
        Buzzer = 1;
        wait(0.001);
        Buzzer = 0;
        }
    wait_ms(500);
    R = 1;
    wait(0.4);
    R = 0;
    G = 1;
    wait(0.4);
    G = 0;
    B = 1;
    wait(0.4);
    R = 1;
    wait(0.4);
    R = 0;
    G = 1;
    wait(0.4);
    R = 0;
    B = 1;
    wait(0.4);
    R = G = B = 0;
    
    if (data_indx >= sample_length){ //resetting the array indx 
        data_indx = 0;
        }
    writer.attach(&write, 10.0);
    
    
    while(1){
        temp_crit = 0;
        light_crit = 0;
        moist_crit = 0;
        badness = 0;
        
        homeo = 1.0;
        
        //moist=ain1.read_u16();          
        moist=ain1.read_u16();          
        wait(0.01); //  ms                     
    
        light = (1 - lght.read())*100.0f;          
        wait(0.01); //  ms
        
        sensor.read();
        cels = sensor.getCelsius();
        hum = sensor.getHumidity();
        wait(0.1);
         
        
        // vyhodnocovanie podmienok
        //podmienky ohrozenia zivota
        
        if(((plant.maxMoist < moist)&& ( moist < plant.minMoist))== false){
            homeo = 0;
            moist_crit = 1;
            }
        if (((plant.minLight < light)&& (light < plant.maxLight)) == false){
            homeo = 0;
            light_crit = 1;
            }
        if (((plant.minCels < cels )&& (cels< plant.maxCels))== false){
            homeo = 0;
            temp_crit = 1;
            }
            
        //podmienky mimo ohrozenia zivota - stav homeo
        
        if((plant.maxMoist < moist)&& (moist<plant.minMoist)){
            homeo = homeo - (((moist - plant.maxMoist)/(plant.minMoist - plant.maxMoist))*weights.m);
            }
        if((plant.minLight < light)&&(light < plant.maxLight)){
            homeo = homeo - (( 1 - ((light - plant.minLight)/(plant.maxLight - plant.minLight)))*weights.l);
            }
        if((plant.minCels < cels)&&(cels < plant.maxCels)){
            homeo = homeo - (( 1 - ((cels - plant.minCels)/(plant.maxCels - plant.minCels)))*weights.c);
            }
        if((plant.minHum < hum)&&(hum < plant.maxHum)){
            homeo = homeo - (( 1 - ((hum - plant.minHum)/(plant.maxHum - plant.minHum)))*weights.h);
            }
          
        if(homeo < 0){
            homeo = 0;
            }
        
        // SIGNALIZACIA STAVU via RGB LED
        if (homeo >= 0.6){
            R = 0;
            G = 1;
            B = 0;
            }
            
        else if ((homeo < 0.6)&&(homeo > 0.2)){
            R = 0;
            G = 1;
            B = 1;
            }
        
        else{
            R = 1;
            G = B = 0;
            }
        
        badness = temp_crit + light_crit + moist_crit;
        SosSignal(R,G,B,badness);
        
        
        // =====choice of terminal visuals====
        // catching = true --> PC-friendly format of data for graphing
        // catching = false --> HUMAN-friendly format of data
        if (catching){
            serial.printf("%d\r\n",moist);
            }
        else{
            serial.printf("\033[2J"); //clear display
            serial.printf("===================\r\n");
            serial.printf("Moisture is:  %d\r\n", moist);
            serial.printf("Light is:     %.2f\r\n", light);
            serial.printf("Temperature: %d Cel\r\n",cels);
            serial.printf("Air humidity:  %d %%\r\n",hum);
            serial.printf("Homeostasis is %.2f\r\n\n",homeo);  
            serial.printf("HOMEOSTASIS-O-METER\r\n\n");
            
            //xxxxxxxx Health Bar xxxxxxxxxxx
            h_length = 20;
            h_magnitude = (int)(homeo *h_length);
            
            
            if(h_magnitude == 0){
            serial.printf("!!! CRITICAL !!!\n\r");
                for (int j = 1; j < h_length+1; j++) {
                    serial.printf("\033[41m");
                    serial.printf(" ");
                    serial.printf("\033[m"); //turn off character attributes
                    //wait(0.01);
                } //for
            }
            else{
                serial.printf("Bad");
                for (int i = 0; i < h_length - 8; i++){
                    serial.printf(" ");
                }
                serial.printf("Good\n\r");
                red_bnd = (int)((h_length/10)*2);
                yel_bnd = (int)((h_length/10)*4) + red_bnd;
                for (int j = 1; j < h_magnitude+1; j++) {
                    if(j<=red_bnd){
                        serial.printf("\033[41m");
                    }
                    if((j>red_bnd)&&(j<yel_bnd)){
                        serial.printf("\033[43m");
                    }
                    if(j >= yel_bnd){
                        serial.printf("\033[42m");
                    }
                    serial.printf(" ");
                    serial.printf("\033[m"); //turn off character attributes
                    //wait(0.01);
                } //for
            }//else
            serial.printf("\n\r");
            //serial.printf("\033[2K"); //clear the entire line  
            //xxxxxxxx Health Bar xxxxxxxxxxx 
            serial.printf("\r\n===================\r\n");
            wait(1);
        }//else
        /*
        
        // == TO BE CONTINUED... == :)
        
        serial.printf("===================\r\n");
        serial.printf("WATERING IN PROGRESS...r\n");
        serial.printf("\r\n===================\r\n");
        
        wait(0.1);
        flowOn = 1;
        wait(5);
        flowOn = 0;
        wait(0.1);
        */
        
        wait(0.01); // ms
        
        
        if (serial.readable()) { //je k dispozici novy prikaz?
            comIN = serial.getc(); //je, tak ho nacti
            switch(comIN){ //a podle toho co prislo, proved danou akci
                case 'c':
                    catching = true;
                    
                break;
                case '*':
                    serial.printf("*"); //sending "I understood your msg."
                    //wait(0.5);
                    for (int i = 0; i < sample_length; i++){
                        serial.printf("%d\n", data_RH[i]);
                        }
                    serial.printf(";\n");
                    for (int i = 0; i < sample_length; i++){
                        serial.printf("%d\n", data_light[i]);
                        }
                    serial.printf(";\n");
                    for (int i = 0; i < sample_length; i++){
                        serial.printf("%d\n", data_moist[i]);
                        }
                    serial.printf(";\n");
                    for (int i = 0; i < sample_length; i++){
                        serial.printf("%d\n", data_temp[i]);
                        }
                    
                     
                break;
                case 'g':
                    catching = false;                    
                break;
                default: 
                    serial.printf("Unknown command!\r\n");
                break;
            };
        }; 
    }
}

