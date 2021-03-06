#include <DHT.h>
#include <U8g2lib.h>
#include<Wire.h>
#include <Time.h>
#include <RTClib.h>

#define DHTTYPE DHT22
#define relayHeatPin 2
#define relayLightPin 3
#define cooler1Pin 6
#define humidifierPin 7

RTC_DS1307 RTC;
DHT dht(9, DHTTYPE);
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);

float temperature;
float humidity;
const char DEGREE_SYMBOL[] = {0xB0,'\0'};
char date [9];
char curTime [6];
unsigned long uptime=0; // variable storing system uptime
int upshift=0; // Shift M(inutes) on the screen, according to number of current system Uptime digits
int minCheck; // check for minutes to count whole system uptime properly
int minTemp = 23;
int maxTemp = 27;
int minHum = 60;
int maxHum = 90;
bool humidState;
bool coolerState;
int checkTemp = minTemp; // Check for temprature treshhold 
int checkCool = maxTemp;
static const unsigned long SEND_INTERVAL = 20000; // ms
static unsigned long lastRefreshTime = 0;
  
void setup() {
  Serial.begin(9600);
  dht.begin();
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.setColorIndex(1);
  Wire.begin();
  RTC.begin();
  minCheck = RTC.now().minute();
  if (! RTC.isrunning())
  {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  pinMode(cooler1Pin, OUTPUT);
  pinMode(relayLightPin, OUTPUT);
  pinMode(relayHeatPin, OUTPUT);
  pinMode(humidifierPin, OUTPUT);
  Serial.println("_____________________________________________________________________");
  Serial.println("|   Date   |  Time |  Temp |  Hum  |Cooler| Heat|Light|Humid| Uptime |");
  Serial.println("_____________________________________________________________________");
}

void loop() {
  u8g2.firstPage();
  do {
    draw();
  } while(u8g2.nextPage());
}

void sendStats(DateTime now) {
  char sendData[33];
  String delim = ";";
  char tempData[18];
  sprintf(tempData, "%02d.%02d.%02d;%02d:%02d;", now.day(), now.month(), now.year() - 2000, now.hour(), now.minute());
  String tempStr = tempData;
  tempStr += (String(temperature)).substring(0, 4);
  tempStr += delim;
  tempStr += int(humidity - 15);
  tempStr += delim;
  tempStr += digitalRead(relayHeatPin);
  tempStr += coolerState;
  tempStr += humidState;
  tempStr += digitalRead(relayLightPin);
  tempStr += delim;
  tempStr += uptime;
  tempStr.toCharArray(sendData, 33);
//  Serial.println(sendData);

  Wire.beginTransmission(9); // transmit to device #1
  Wire.write(sendData);              // sends x 
  Wire.endTransmission();    // stop transmitting
}
  
void draw(){
  DateTime now = RTC.now();
  
  if(millis() - lastRefreshTime >= SEND_INTERVAL) {
    calculateDeltaTime(now); // getting system Uptime and setting shift for M(inutes)
    if(millis() - lastRefreshTime >= SEND_INTERVAL / 4) {
//      sendStats(now);
    }
    lastRefreshTime += SEND_INTERVAL;
  }

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
  u8g2.print(humidity - 15, 0);
  u8g2.drawStr(116, 20, "%");

  u8g2.drawStr(2, 30, "Humidifier:");
  if (humidity < minHum + 15) { // ADDED DIFF COMPARING TO ANOTHER SENSOR
    u8g2.drawStr(56, 30, "ON");
    digitalWrite(humidifierPin, HIGH);
    humidState = true;
    delay(2000);
  } else if ((now.hour() == 06 and now.minute() == 58) or (now.hour() == 01 and now.minute() == 04)) { // turning on humidifier twice per day
    u8g2.drawStr(56, 30, "ON");
    digitalWrite(humidifierPin, HIGH); 
    humidState = true;
  } else {
    u8g2.drawStr(56, 30, "OFF");
    digitalWrite(humidifierPin,LOW);
    humidState = false;
  }

  u8g2.drawStr(74, 30, "|Vent.:");
  if (temperature > checkCool) {
  u8g2.drawStr(107, 30, "ON");
  if (digitalRead(cooler1Pin) == LOW){
    digitalWrite(cooler1Pin,HIGH);
    coolerState = true;
    checkCool -= 1;
    delay(2000);
    }
  } else if ((now.hour() == 06 and now.minute() == 56) or (now.hour() == 01 and now.minute() == 04)) { // turning on ventilation twice per day
    u8g2.drawStr(107, 30, "ON");
    digitalWrite(cooler1Pin,HIGH);
    coolerState = true;  
  } else {
    u8g2.drawStr(107, 30, "OFF");
    digitalWrite(cooler1Pin,LOW);
    coolerState = false;
    checkCool = maxTemp;
  }

  u8g2.drawStr(2, 40, "Heating:");
  if (temperature < checkTemp) {
    u8g2.drawStr(56, 40, "ON");
    if (temperature < minTemp && checkTemp == minTemp){
      digitalWrite(relayHeatPin,HIGH);
      checkTemp += 1;
    }
  } else {
    u8g2.drawStr(56, 40, "OFF");
    digitalWrite(relayHeatPin,LOW);
    checkTemp = minTemp;
  }

  u8g2.drawUTF8(74, 40, "|Light :");
  if (now.hour() >= 07 and now.hour() < 17) {
    digitalWrite(relayLightPin,HIGH);
    u8g2.drawStr(107, 40, "ON");
  } else {
    digitalWrite(relayLightPin,LOW);
    u8g2.drawStr(107, 40, "OFF");
  }

  u8g2.drawHLine(1, 42, 127);
  u8g2.drawStr(2, 52, "TEMP");
  u8g2.drawStr(31, 52, "|Min:");
  u8g2.setCursor(58, 52);
  u8g2.print(minTemp);
  if (checkTemp != minTemp) {
    u8g2.drawStr(69, 52, "T"); //Identificator for treshhold heating
  }
  u8g2.drawStr(74, 52, "|Max:");
  u8g2.setCursor(107, 52);
  u8g2.print(maxTemp);
  if (checkCool != maxTemp) {
    u8g2.drawStr(119, 52, "T"); //Identificator for treshhold cooling
  }
  u8g2.drawStr(2, 62, "HUM");
  u8g2.drawStr(31, 62, "|Min:");
  u8g2.setCursor(58, 62);
  u8g2.print(minHum);
  u8g2.drawStr(74, 62, "|Max:");
  u8g2.setCursor(107, 62);
  u8g2.print(maxHum);
  u8g2.drawVLine(32, 42, 5);
  u8g2.drawVLine(75, 42, 5);
}

void calculateDeltaTime(DateTime now) {
  if (minCheck != now.minute()) {
    uptime += 1;
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

  minCheck = now.minute();
  Serial.print("| ");
  Serial.print(date);
  Serial.print(" | ");
  Serial.print(curTime);
  Serial.print(" | ");
  Serial.print(temperature);
  Serial.print(" | ");
  Serial.print(humidity - 15);
  Serial.print(" |  ");
  Serial.print(coolerState);
  Serial.print("   |  ");
  Serial.print(digitalRead(relayHeatPin));
  Serial.print("  |  ");
  Serial.print(digitalRead(relayLightPin));
  Serial.print("  |  ");
  Serial.print(humidState);
  Serial.print("  |  ");
  Serial.print(uptime);
  Serial.println("  |");
}
