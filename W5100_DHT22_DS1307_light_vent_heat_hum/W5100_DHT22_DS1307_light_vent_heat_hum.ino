#include <DHT.h>
#include <Time.h>
#include <RTClib.h>
#include <Ethernet.h>

#define DHTTYPE DHT22
#define relayHeatPin 2
#define relayLightPin 3
#define coolerPin 6
#define humidifierPin 7

RTC_DS1307 RTC;
DHT dht(9, DHTTYPE);
EthernetServer server(80);
IPAddress ip(192, 168, 1, 100);
String deviceId = "vE88ABDC967551EF";// "vE88ABDC967551EF"; //or vFEEC96DE2707EDF
const char* logServer = "api.pushingbox.com";
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
float temperature;
float humidity;
char date [9];
char curTime [9];
unsigned long uptime=0; // variable storing system uptime
int upshift=0; // Shift M(inutes) on the screen, according to number of current system Uptime digits
int minCheck; // check for minutes to count whole system uptime properly
int minTemp = 25.5;
int maxTemp = 27;
int minHum = 70;
int maxHum = 90;
int humidState;
int coolerState;
int checkTemp = minTemp; // Check for temprature treshhold 
int checkCool = maxTemp;
static const unsigned long SEND_INTERVAL = 20000;
static unsigned long lastRefreshTime = 0;
String serverMsg = "Nothing to send yet";
  
void setup() {
  Serial.begin(9600);
  dht.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  minCheck = RTC.now().minute();
  if (! RTC.isrunning())
  {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  delay(2000);
  //Ethernet.begin(mac);
  delay(1000);
  //Serial.print("Ethernet IP: ");
  //Serial.println(Ethernet.localIP());
  //server.begin();
  
  pinMode(coolerPin, OUTPUT);
  pinMode(relayLightPin, OUTPUT);
  pinMode(relayHeatPin, OUTPUT);
  pinMode(humidifierPin, OUTPUT);
  String beginMsg = "Started with the following ranges: \nTemperature from " + String(minTemp) + " to " + String(maxTemp) + "\nHumidity from " + String(minHum) + " to " + String(maxHum);
  Serial.println(beginMsg);
  Serial.println("Visual system test for:\n1. Cooler");
  digitalWrite(coolerPin, HIGH);
  delay(5000);
  digitalWrite(coolerPin, LOW);
  Serial.println("2. Humidifier");
  digitalWrite(humidifierPin, HIGH);
  delay(5000);
  digitalWrite(humidifierPin, LOW);
  Serial.println("3. Light");
  digitalWrite(relayLightPin, HIGH);
  delay(5000);
  digitalWrite(relayLightPin, LOW);
  Serial.println("4. Heat pad");
  digitalWrite(relayHeatPin, HIGH);
  delay(5000);
  digitalWrite(relayHeatPin, LOW);
  delay(2000);
  Serial.print("|   Date   |   Time   |  Temp |  Hum  |Cooler| Heat|Light|Humid| Uptime |");
}

void loop() {
  DateTime now = RTC.now();
  sprintf(date, "%02d.%02d.%02d", now.day(), now.month(), now.year() - 2000);
  sprintf(curTime, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  
  temperature = dht.readTemperature();
  humidity = dht.readHumidity() - 25;

  if(millis() - lastRefreshTime >= SEND_INTERVAL) {
    calculateDeltaTime(now); // getting system Uptime and setting shift for M(inutes)
    if(millis() - lastRefreshTime >= SEND_INTERVAL / 4) {
      handleParams(now);
      handleNotifs();
     // refreshServer();
    }
    lastRefreshTime += SEND_INTERVAL;
  } 
}

void handleNotifs() {
  String msg;
  if ((temperature > 0.0) && (humidity > 0.0)) {
    if (temperature <= minTemp - 1.0) {
      msg = "Temperature is below minimum: ";
      msg += String(temperature);
    } else if (temperature >= maxTemp + 1.0) {
      msg = "Temperature is above maximum: ";
      msg += String(temperature);
    }
    if (msg.length() > 0 && (humidity == 100 || humidity < minHum - 5.0)) { msg += "; "; }

    if (humidity == 100){
      msg += "Humidity is above maximum: ";
      msg += String(humidity);
    } else if (humidity < minHum - 5.0) {
      msg += "Humidity is below minimum: ";
      msg += String(humidity);
    }
  }  else {
    msg = "No data from MASTER";
  }
  //sendToPushingBox(msg);
  if (msg.length() > 0) {
    serverMsg = "Last notification sent at " + String(date) + " " + String(curTime) + ", message: " + msg;
  }
}

void handleParams(DateTime now){
  if (humidity < minHum) { // REMOVED DIFF COMPARING TO ANOTHER SENSOR
    digitalWrite(humidifierPin, HIGH);
    humidState = 1;
    delay(8000);
  } else if (now.minute() == 59) { // turning on humidifier every hour
    digitalWrite(humidifierPin, HIGH); 
    humidState = 1;
  } else {
    digitalWrite(humidifierPin,LOW);
    humidState = 0;
  }

  if (temperature > checkCool) {
    if (coolerState == 0){
      digitalWrite(coolerPin,HIGH);
      coolerState = 1;
      checkCool -= 1;
      delay(2000);
    }
  } else if ((now.hour() == 07 and now.minute() == 56) or (now.hour() == 16 and now.minute() == 57)) { // turning on ventilation twice per day
    digitalWrite(coolerPin,HIGH);
    coolerState = 1;  
  } else {
    digitalWrite(coolerPin,LOW);
    coolerState = 0;
    checkCool = maxTemp;
  }

  if (temperature < checkTemp) {
    if (temperature < minTemp && checkTemp == minTemp){
      digitalWrite(relayHeatPin,HIGH);
      checkTemp += 1;
    }
  } else {
    digitalWrite(relayHeatPin,LOW);
    checkTemp = minTemp;
  }

  if (now.hour() >= 07 and now.hour() < 17) {
    digitalWrite(relayLightPin,HIGH);
  } else {
    digitalWrite(relayLightPin,LOW);
  }
}

String handleSensors(int data) {
  if (data == 1) { return "ON "; } else { return "OFF"; }
}

void refreshServer() {
  EthernetClient client = server.available();
  Serial.print("Here");
  if (client) {
    Serial.print(" Web started ");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println("Refresh: 10"); 
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print(serverMsg);
          client.println("<br />");          
          client.print("Date: ");
          client.print(date);
          client.println("<br />");
          client.print("Time: ");
          client.print(curTime);
          client.println("<br />");
          client.print("Uptime: ");
          client.print(uptime);
          client.println("<br />");
          client.print("Temperature: ");
          client.print(temperature);
          client.println("<br />");
          client.print("Humidity: ");
          client.print(humidity);
          client.println("<br />");
          client.print("Heat: ");
          client.print(handleSensors(digitalRead(relayHeatPin)));
          client.println("<br />");
          client.print("Cooler: ");
          client.print(handleSensors(coolerState));
          client.println("<br />");
          client.print("Humidifier: ");
          client.print(handleSensors(humidState));
          client.println("<br />");
          client.print("Light: ");
          client.print(handleSensors(digitalRead(relayLightPin)));
          client.println("<br />");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
    Serial.print("and disconnected");
  }
}

void sendToPushingBox(String message) {
  if (message.length() > 0) {
    delay(3000);
    EthernetClient client2;
    Serial.print(" HERE2 ");
    if (client2.connect("api.pushingbox.com", 80)) {            
      String postStr = "devid=";
      postStr += String(deviceId);
      postStr += "&message_param=";
      postStr += message;
      postStr += "\r\n\r\n";

      client2.print("POST /pushingbox HTTP/1.1\n");
      client2.print("Host: api.pushingbox.com\n");
      client2.print("Connection: close\n");
      client2.print("Content-Type: application/x-www-form-urlencoded\n");
      client2.print("Content-Length: ");
      client2.print(postStr.length());
      client2.print("\n\n");
      client2.print(postStr);
      Serial.print(" POST sent with: ");
      Serial.print(message);
    }
    client2.stop();
    delay(2000);
  }
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
  Serial.println();
  Serial.print("| ");
  Serial.print(date);
  Serial.print(" | ");
  Serial.print(curTime);
  Serial.print(" | ");
  Serial.print(temperature);
  Serial.print(" | ");
  Serial.print(humidity);
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
  Serial.print("  |");  
  Serial.print(serverMsg);
}
