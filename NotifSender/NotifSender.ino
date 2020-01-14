// Include the required Wire library for I2C<br>#include 
#include<Wire.h>
               //
char mystr[60] = "10.10.2020;11:00;9999;23;99;0101";//;off;off;off;off"; //String data
void setup() {
  // Start the I2C Bus as Master
  Serial.begin(9600);
  Wire.begin(); 
}
void loop() {
  Wire.beginTransmission(9); // transmit to device #1
  Wire.write(mystr);              // sends x 
  Wire.endTransmission();    // stop transmitting
  delay(10000);
  Serial.println(mystr);
}
