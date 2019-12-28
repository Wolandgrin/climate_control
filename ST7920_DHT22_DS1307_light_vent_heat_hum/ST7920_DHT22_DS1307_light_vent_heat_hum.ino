#include <DHT.h>
#include <U8g2lib.h>
#include<Wire.h>
#include <Time.h>
#include "RTClib.h"
#define DHTTYPE DHT22
#define relayHeatPin 2
#define relayLightPin 3
#define relayCooler1Pin 6
//#define relayCooler2Pin 5
#define relayHumidifierPin 7

RTC_DS1307 RTC;
DHT dht(9, DHTTYPE);
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);

float temperature;
float humidity;
const char DEGREE_SYMBOL[] = {0xB0,'\0'};
char date [9];
char curTime [6];
unsigned long uptime=0;
unsigned long oldTime=0;
int upshift=0;
int minCheck;
float minTemp = 23;
float maxTemp = 25;
float minHum = 90;
float maxHum = 100;
bool workingMode = false;

void setup() {
  Serial.begin(9600);
  dht.begin();
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.setColorIndex(1);
  Wire.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  RTC.begin();
  minCheck = RTC.now().minute();
  pinMode(relayCooler1Pin, OUTPUT);
//  pinMode(relayCooler2Pin, OUTPUT);
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

  u8g2.drawUTF8(74, 10, "|UP:");
  u8g2.setCursor(94, 10);
  u8g2.print(uptime);
  u8g2.drawStr(100 + upshift, 10, "M");
  
  u8g2.drawStr(2, 20, "Temp.:");
  u8g2.setCursor(38, 20);
  u8g2.print(temperature, 1);
  u8g2.drawUTF8(59, 20, DEGREE_SYMBOL);
  u8g2.drawStr(64, 20, "C");

  u8g2.drawStr(74, 20, "|Hum:");
  u8g2.setCursor(103, 20);
  u8g2.print(humidity, 0);
  u8g2.drawStr(116, 20, "%");
  
  u8g2.drawStr(2, 30, "Humidifier:");
  if (humidity < minHum) {
    digitalWrite(relayHumidifierPin, HIGH);
    u8g2.drawStr(56, 30, "ON");
  } else {
    digitalWrite(relayHumidifierPin,LOW);
    u8g2.drawStr(56, 30, "OFF");
  }
  
  u8g2.drawStr(74, 30, "|Vent.:");
  if (temperature > maxTemp) {
    u8g2.drawStr(107, 30, "ON");
    digitalWrite(relayCooler1Pin,HIGH);
//    digitalWrite(relayCooler2Pin,HIGH);
  } else {
    u8g2.drawStr(107, 30, "OFF");
    digitalWrite(relayCooler1Pin,LOW);
//    digitalWrite(relayCooler2Pin,LOW);
  }

  u8g2.drawStr(2, 40, "Heating:");
  if (temperature < minTemp) {
    u8g2.drawStr(56, 40, "ON");
    digitalWrite(relayHeatPin,HIGH);
 } else {
    u8g2.drawStr(56, 40, "OFF");
   digitalWrite(relayHeatPin,LOW);
 }
  u8g2.drawUTF8(74, 40, "|Light :");
  if (now.minute() > 00 and now.minute() < 15){
    digitalWrite(relayLightPin,HIGH);
    u8g2.drawStr(107, 40, "ON");
  } else {
    digitalWrite(relayLightPin,LOW);
    u8g2.drawStr(107, 40, "OFF");
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
    } else if (uptime > 99 and uptime < 1000) {
      upshift = 12;
    } else if (uptime > 999 and uptime < 10000) {
      upshift = 18;
    } else if (uptime > 9999) {
      upshift = 24;
    }
  }
}
