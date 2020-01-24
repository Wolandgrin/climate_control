// Include the required Wire library for I2C<br>#include 
#include<Wire.h>

char date [10] = "10.10.2020";
char curTime [5] = "11:00";
unsigned long uptime=9898;
float temperature = 33.52;
float humidity = 87.93;
#define relayHeatPin 2
#define relayLightPin 3
#define cooler1Pin 6
#define humidifierPin 7

char sendData[31]= "10.10.20;11:00;28;na;0101;9999";//;off;off;off;off"; //String data
void setup() {
  // Start the I2C Bus as Master
  Serial.begin(9600);
//  Serial.begin(115200);
  Serial.println("Started!");
//  Wire.begin(); 
//  digitalWrite(cooler1Pin, LOW); 
//  digitalWrite(humidifierPin, HIGH);
//  digitalWrite(relayHeatPin, LOW); 
//  digitalWrite(relayLightPin, HIGH);
}
void loop() {
//  char sendData[45];
//  char float_buf[45];
//  String delim = ";";
//  String tempStr = String(date + delim + curTime + delim);
////  sprintf(tempStr, "%02d:%02d:%02d;", , MIN, SEC);
//
//  tempStr += int(temperature);
//  tempStr += ";";
//  tempStr += int(humidity);
//  tempStr += ";";
//  tempStr += digitalRead(cooler1Pin);
//  tempStr += digitalRead(humidifierPin);
//  tempStr += digitalRead(relayHeatPin);
//  tempStr += digitalRead(relayLightPin);
//  tempStr += ";";
//  tempStr += uptime;
//
//  
//  tempStr.toCharArray(sendData, 45);
//  strcat(sendData, date);
//  strcat(sendData, ";");
//
//  strcat(sendData, curTime);
//  strcat(sendData, ";");
  
//  dtostrf(uptime, 16, 5, float_buf);
//  strcat(sendData, float_buf);
//  strcat(sendData, ";");
//
//  
//  dtostrf(temperature, 16, 2, float_buf);
//  strcat(sendData, float_buf);
//  strcat(sendData, ";");
//  
//  dtostrf(humidity, 16, 2, float_buf);
//  strcat(sendData, float_buf);
//  strcat(sendData, ";");
//  
//  dtostrf(cooler1Pin, 16, 1, float_buf);
//  strcat(sendData, float_buf);
//  dtostrf(humidifierPin, 16, 1, float_buf);
//  strcat(sendData, float_buf);
//  dtostrf(relayHeatPin, 16, 1, float_buf);
//  strcat(sendData, float_buf);
//  dtostrf(relayLightPin, 16, 1, float_buf);
//  strcat(sendData, float_buf);
  Serial.write(sendData,31); //Write the serial data
  Serial.println();

//  Wire.beginTransmission(9); // transmit to device #1
//  Wire.write(sendData);              // sends x 
//  Wire.endTransmission();    // stop transmitting
  delay(10000);
}
