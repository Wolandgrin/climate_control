#include <DHT.h>
#include <Time.h>
#include <RTClib.h>
#include <Ethernet.h>
#include <MQUnifiedsensor.h>

#define DHTTYPE DHT22
#define rHeatP 2
#define rLightP 3
#define ventP 6
#define humidP 7
#define type "MQ-135"
RTC_DS1307 RTC;
DHT dht(9, DHTTYPE);
EthernetServer srv(80);
String devId = "vE88ABDC967551EF";// "vE88ABDC967551EF"; //or vFEEC96DE2707EDF
const char* logSrv = "api.pushingbox.com";
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
float temp;
float hum;
char date [9];
char curT [9];
unsigned long upTime=0; // variable storing system uptime
int minCheck; // check for minutes to count whole system uptime properly
int minTmp = 25.5;
int maxTmp = 27;
int minHum = 70;
int maxHum = 90;
int maxCO2 = 200;
int hState;
int vState;
int checkTemp = minTmp; // Check for temprature treshhold 
int checkCool = maxTmp;
static unsigned long refrT = 0;
String srvMsg = "None";
int airQ;
String ms;
int defDel = 5000;

void setup() {
  Serial.begin(9600);
  dht.begin();
  RTC.begin();
//  RTC.adjust(DateTime(__DATE__, __TIME__));
  minCheck = RTC.now().minute();
  if (! RTC.isrunning())
  {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  delay(2000);
  Ethernet.begin(mac);
  delay(1000);
  Serial.println(Ethernet.localIP());
  srv.begin();
  
  pinMode(ventP, OUTPUT);
  pinMode(rLightP, OUTPUT);
  pinMode(rHeatP, OUTPUT);
  pinMode(humidP, OUTPUT);
  Serial.println("Ranges: \nTemp: " + String(minTmp) + "-" + String(maxTmp) + "\nHum: " + String(minHum) + "-" + String(maxHum));
  Serial.println("Test:\nCooler");
  digitalWrite(ventP, HIGH);
  delay(defDel);
  digitalWrite(ventP, LOW);
  Serial.println("Humidif");
  digitalWrite(humidP, HIGH);
  delay(defDel);
  digitalWrite(humidP, LOW);
  Serial.println("Light");
  digitalWrite(rLightP, HIGH);
  delay(defDel);
  digitalWrite(rLightP, LOW);
  Serial.println("Heat");
  digitalWrite(rHeatP, HIGH);
  delay(defDel);
  digitalWrite(rHeatP, LOW);
  delay(2000);
  Serial.print("|  Date  |  Time  | Temp| Hum |Air|C|H|H|L|Uptime|");
}

void loop() {
  DateTime now = RTC.now();
  sprintf(date, "%02d.%02d.%02d", now.day(), now.month(), now.year() - 2000);
  sprintf(curT, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  
  temp = dht.readTemperature();
  hum = dht.readHumidity(); // REMOVED DIFF COMPARING TO ANOTHER SENSOR
  airQ = analogRead(0);

  if(minCheck != now.minute()) {
    handleNotifs();
  }
  if(millis() - refrT >= defDel * 2) { // every 20 sec
    calcUptime(now); // getting system Uptime and setting shift for M(inutes)
    if(millis() - refrT >= defDel) {
      handleParams(now);
      reloadPage();
    }
    refrT += defDel * 2;
  } 
}

void handleNotifs() {
  String msg;
  if ((temp > 0.0) && (hum > 0.0)) {
    if (temp <= minTmp - 1.0) {
      msg = " Low temp: " + String(temp);
    } else if (temp >= maxTmp + 1.0) {
      msg = "; High temp " + String(temp);
    }

    if (hum < minHum - 5.0) {msg += String("; Low hum: " + String(hum));}

    if (airQ > maxCO2 + 10.0) {msg += String("; High CO2: " + String(airQ));}

  }  else {
    msg = "No data";
  }    
  
  if (msg.length() > 4) {
    srvMsg = "Sent at " + String(date) + " " + String(curT);
    pushNotif(msg);
    ms = msg;
  }
}

void handleParams(DateTime now) {
  if (hum < minHum) {
    digitalWrite(humidP, HIGH);
    hState = 1;
    delay(defDel * 2);
  } else if (now.minute() > 57) { // turn on humidif every hour
    digitalWrite(humidP, HIGH); 
    hState = 1;
  } else {
    digitalWrite(humidP, LOW);
    hState = 0;
  }

  if (temp > checkCool) {
    if (vState == 0) {
      digitalWrite(ventP, HIGH);
      vState = 1;
      checkCool -= 1;
      delay(2000);
    }
  } else if (((now.hour() == 07 or now.hour() == 16) and now.minute() == 57) or airQ > maxCO2) { // turn on vent twice per day
    digitalWrite(ventP, HIGH);
    vState = 1;  
  } else {
    digitalWrite(ventP, LOW);
    vState = 0;
    checkCool = maxTmp;
  }

  if (temp < checkTemp){
    if (temp < minTmp && checkTemp == minTmp) {
      digitalWrite(rHeatP, HIGH);
      checkTemp += 1;
    }
  } else {
    digitalWrite(rHeatP, LOW);
    checkTemp = minTmp;
  }

  if (now.hour() >= 07 and now.hour() < 17) {
    digitalWrite(rLightP, HIGH);
  } else {
    digitalWrite(rLightP, LOW);
  }
}

String hndlSens(int data) {
  if (data == 1){return "ON ";} else {return "OFF";}
}

void reloadPage() {
  EthernetClient client = srv.available();
  if (client){
    Serial.print(" Web on ");
    boolean blankLine = true;
    while (client.connected()){
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && blankLine){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println("Refresh: 10"); 
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print(srvMsg);
          client.print(ms);
          client.print("<br />");
          client.print("Date: " + String(date) + "<br />");
          client.print("Time: " + String(curT) + "<br />");
          client.print("Uptime: " + String(upTime) + "<br />");
          client.print("Temp: " + String(temp) + "<br />");
          client.print("Hum: " + String(hum) + "<br />");
          client.print("CO2: " + String(airQ) + "<br />");
          client.print("Heater: " + hndlSens(digitalRead(rHeatP)) + "<br />");
          client.print("Cooler: " + hndlSens(vState) + "<br />");
          client.print("Humidifier: " + hndlSens(hState) + "<br />");
          client.print("Light: " + hndlSens(digitalRead(rLightP)) + "<br />");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          blankLine = true;
        } else if (c != '\r') {
          blankLine = false;
        }
      }
    }
    delay(1);
    client.stop();
    Serial.print("and off ");
  }
}

void pushNotif(String msg) {
  delay(2000);
  EthernetClient client2;
  if (client2.connect(logSrv, 80)) {            
    String postStr = "devid=";
    postStr += devId;
    postStr += "&message_param=";
    postStr += msg;
    postStr += "\r\n\r\n";

    client2.print("POST /pushingbox HTTP/1.1\n");
    client2.print("Host: " + String(logSrv) + "\n");
    client2.print("Connection: close\n");
    client2.print("Content-Type: application/x-www-form-urlencoded\n");
    client2.print("Content-Length: ");
    client2.print(postStr.length());
    client2.print("\n\n");
    client2.print(postStr);
  }
  client2.stop();
  delay(defDel);
}

void calcUptime(DateTime now) {
  if (minCheck != now.minute()) {
    upTime += 1;
    minCheck = now.minute();
  }
  Serial.println();
  Serial.print("|");
  Serial.print(date);
  Serial.print("|");
  Serial.print(curT);
  Serial.print("|");
  Serial.print(temp);
  Serial.print("|");
  Serial.print(hum);
  Serial.print("|");
  Serial.print(airQ);
  Serial.print("|");
  Serial.print(vState);
  Serial.print("|");
  Serial.print(digitalRead(rHeatP));
  Serial.print("|");
  Serial.print(hState);
  Serial.print("|");
  Serial.print(digitalRead(rLightP));
  Serial.print("|");
  Serial.print(upTime);
  Serial.print("|");
  Serial.print(ms);
}
