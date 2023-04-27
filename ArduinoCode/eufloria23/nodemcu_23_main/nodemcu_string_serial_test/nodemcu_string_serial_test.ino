#define BUFFER_SIZE 64

char buffer[BUFFER_SIZE];
int bufferIndex = 0;

struct Data{
  int tmp = 0;
  int hum = 0;
  int dst = 0;
  int wet = 0;
  int lgh = 0;
  int co2 = 0;
  bool pmp = false;
  } data;

void setup() {
  Serial.begin(9600);
}
//STA\rhum,tmp,wet,lgh,dst,co2,pmp\r045,022,8264,1000,1000,0485,0\n

struct Data parseData(String payload, Data data) {
  int commaIndex;
  
  // Extract humidity value
  commaIndex = payload.indexOf(",");
  if (commaIndex != -1) {
    String humStr = payload.substring(0, commaIndex);
    payload.remove(0, commaIndex + 1);
    data.hum = humStr.toInt();
  }
  
  // Extract temperature value
  commaIndex = payload.indexOf(",");
  if (commaIndex != -1) {
    String tmpStr = payload.substring(0, commaIndex);
    payload.remove(0, commaIndex + 1);
    data.tmp = tmpStr.toInt();
  }
  
  // Extract wetness value
  commaIndex = payload.indexOf(",");
  if (commaIndex != -1) {
    String wetStr = payload.substring(0, commaIndex);
    payload.remove(0, commaIndex + 1);
    data.wet = wetStr.toInt();
  }
  
  // Extract light value
  commaIndex = payload.indexOf(",");
  if (commaIndex != -1) {
    String lghStr = payload.substring(0, commaIndex);
    payload.remove(0, commaIndex + 1);
    data.lgh = lghStr.toInt();
  }
  
  // Extract distance value
  commaIndex = payload.indexOf(",");
  if (commaIndex != -1) {
    String dstStr = payload.substring(0, commaIndex);
    payload.remove(0, commaIndex + 1);
    data.dst = dstStr.toInt();
  }
  
  // Extract CO2 value
  commaIndex = payload.indexOf(",");
  if (commaIndex != -1) {
    String co2Str = payload.substring(0, commaIndex);
    payload.remove(0, commaIndex + 1);
    data.co2 = co2Str.toInt();
  }
  
  // Extract pump value
  commaIndex = payload.indexOf("\r");
  if (commaIndex != -1) {
    String pmpStr = payload.substring(0, commaIndex);
    payload.remove(0, commaIndex + 1);
    data.pmp = (pmpStr == "1");
  }

  return data;
}



void loop() {
  bool finished_reading = false;
  while (!finished_reading){
    if (Serial.available() > 0) {
      char incomingByte = Serial.read();
  
      if (incomingByte == '\n') {
        // Null-terminate the buffer
        buffer[bufferIndex] = '\0';
  
        // Print the received string
        Serial.println(buffer);
  
        // Reset the buffer index
        bufferIndex = 0;
        finished_reading = true;
      } else {
        // Add the incoming byte to the buffer
        buffer[bufferIndex++] = incomingByte;
  
        // Check for buffer overflow
        if (bufferIndex >= BUFFER_SIZE) {
          Serial.println("Buffer overflow!");
          bufferIndex = 0;
          finished_reading = true;
        }
      }
    }
  }
  String str_buffer(buffer);
  data = Data();
  data = parseData(str_buffer, data);

  Serial.print("example data: hum: ");
  Serial.print(data.hum);
  Serial.print(" tmp: ");
  Serial.print(data.hum);
  Serial.print(" pump on: ");
  Serial.println(data.pmp);
  
}
