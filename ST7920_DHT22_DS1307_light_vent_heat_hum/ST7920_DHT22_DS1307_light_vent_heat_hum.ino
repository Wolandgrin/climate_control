#include <DHT.h>
#include <U8g2lib.h>
#include<Wire.h>
#include <Time.h>
#include "RTClib.h"
#define DHTTYPE DHT22
RTC_DS1307 RTC;
DHT dht(7, DHTTYPE);
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);
//#ifdef U8X8_HAVE_HW_SPI
//#include <SPI.h>
//#endif
//#ifdef U8X8_HAVE_HW_I2C
//#include <Wire.h>
//#endif
float temperature;
float humidity;
const char DEGREE_SYMBOL[] = {0xB0,'\0'};
char date [9];
char curTime [6];
unsigned long uptime=0;
unsigned long oldTime=0;
int upshift=0;
int minCheck;
int relayCooler1Pin=1;
int relayCooler2Pin=2;
int relayLightPin=3;
int relayHeatPin=4;
int relayHumidifierPin=5;

void setup() {
  dht.begin();
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.setColorIndex(1);
  Serial.begin(9600);
  Wire.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  RTC.begin();
  minCheck = RTC.now().minute();
  pinMode(relayCooler1Pin, OUTPUT);
  pinMode(relayCooler2Pin, OUTPUT);
  pinMode(relayLightPin, OUTPUT);
  pinMode(relayHeatPin, OUTPUT);
  pinMode(relayHumidifierPin, OUTPUT);
  if (! RTC.isrunning())
  {
    Serial.println("RTC is NOT running!");
  }
}

void loop() {
  u8g2.firstPage();
  do {
    draw();
//    delay(3000);
  } while(u8g2.nextPage());
}
  
void draw(){
  DateTime now = RTC.now();
  calculateDeltaTime(now);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  u8g2.drawFrame(0,0,128,64);

  sprintf(date, "%02d.%02d.%02d", now.day(), now.month(), now.year() - 2000);
  sprintf(curTime, "%02d:%02d", now.hour(), now.minute());
  u8g2.drawUTF8(2, 10, date);
  u8g2.drawUTF8(46, 10, curTime);
  u8g2.drawStr(2, 20, "Temp.:");
  u8g2.setCursor(38, 20);
  u8g2.print(temperature, 1);
  u8g2.drawUTF8(59, 20, DEGREE_SYMBOL);
  u8g2.drawStr(65, 20, "C");

  u8g2.drawUTF8(73, 10, "|UP:");
  u8g2.setCursor(94, 10);
  u8g2.print(uptime);
  u8g2.drawStr(100 + upshift, 10, "M");
  u8g2.drawStr(73, 20, "|Vent.:");
  if (temperature > 29) {
    u8g2.drawStr(107, 20, "ON");
    digitalWrite(relayCooler1Pin,HIGH);
    digitalWrite(relayCooler2Pin,HIGH);
  } else {
    u8g2.drawStr(107, 20, "OFF");
    digitalWrite(relayCooler1Pin,LOW);
    digitalWrite(relayCooler2Pin,LOW);
  }

  u8g2.drawStr(2, 30, "Hum.:");
  u8g2.setCursor(38, 30);
  u8g2.print(humidity, 1);
  u8g2.drawStr(61, 30, "%");

  u8g2.drawStr(73, 30, "|Heat.:");
  if (temperature < 27) {
    u8g2.drawStr(107, 30, "ON");
    digitalWrite(relayHeatPin,HIGH);
 } else {
    u8g2.drawStr(107, 30, "OFF");
   digitalWrite(relayHeatPin,LOW);
 }
  u8g2.drawUTF8(73, 40, "|Light:");
  if (now.hour() == 20){
    digitalWrite(relayLightPin,HIGH);
    u8g2.drawStr(107, 40, "ON");
  } else {
    digitalWrite(relayLightPin,LOW);
    u8g2.drawStr(107, 40, "OFF");
  }
  u8g2.drawStr(73, 50, "|Aqua:");
  if (humidity < 70) {
    digitalWrite(relayHumidifierPin, HIGH);
    u8g2.drawStr(107, 50, "ON");
  } else {
    digitalWrite(relayHumidifierPin,LOW);
    u8g2.drawStr(107, 50, "OFF");
  }
}

void calculateDeltaTime(DateTime now){
  if (minCheck != now.minute()) {
    uptime += 1;
    minCheck = now.minute();
    Serial.print("uptime: ");
    Serial.println(uptime);
    Serial.print("temp: ");
    Serial.println(temperature);
    Serial.print("hum: ");
    Serial.println(humidity);
    Serial.print("Coolers relay: ");
    Serial.println(digitalRead(relayCooler1Pin));
    Serial.print("Heat relay: ");
    Serial.println(digitalRead(relayHeatPin));
    Serial.print("Light relay: ");
    Serial.println(digitalRead(relayLightPin));
    Serial.print("Humidifier relay: ");
    Serial.println(digitalRead(relayHumidifierPin));
    Serial.println("___________________________");
    if (uptime > 9 and uptime < 100) {
      upshift = 6;
    } else if (uptime > 99) {
      upshift = 12;
    } else if (uptime > 999) {
      upshift = 22;
    }
  }
}
