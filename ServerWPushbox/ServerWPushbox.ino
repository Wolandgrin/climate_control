#include<Wire.h>
#include <Ethernet.h>

String deviceId = "vE88ABDC967551EF";// "vE88ABDC967551EF"; //or vFEEC96DE2707EDF
const char* logServer = "api.pushingbox.com";
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x09};
char receivedData[31]; 
String curDate;
String curTime;
String uptime;
String temp;
String humid;
String heat;
String cooler;
String humidif;
String light;
static const unsigned long SEND_INTERVAL = 60000; // ms
static unsigned long lastRefreshTime = 0;
  
EthernetServer server(80);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }
  Wire.begin(9); 
  Wire.onReceive(receiveEvent);
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("DHCP failed");
    return;
  }
  delay(2000);
  Serial.print("Ethernet IP: ");
  Serial.println(Ethernet.localIP());
  server.begin();
}

void loop() {
  receiver(receivedData);
  delay(700);
  refreshServer();
  delay(700);
  handleNotifs();
  delay(600);
}

void handleNotifs() {
  if(millis() - lastRefreshTime >= SEND_INTERVAL) {
    float t = temp.toFloat();
    long int h = humid.toInt();
    String msg;
    if (t > 0 && h < 0) {
      if (t <= 22) {
        msg = "Temperature is below minimum: ";
        msg += String(temp);
  //      Serial.println(t);
      } else if (t >= 27) {
        msg = "Temperature is above maximum: ";
        msg += temp;
      }
      if (msg.length() > 0 && (h >= 99 || h < 60)) { msg += "; "; }
      
      if (h >= 99){
        msg += "Humidity is above maximum: ";
        msg += String(humid);
      } else if (h < 60) {
        msg += "Humidity is below minimum: ";
        msg += String(humid);
      }
    }  else {
      msg = "No data from MASTER";
    }
    lastRefreshTime += SEND_INTERVAL;
    sendToPushingBox(msg);
//    Serial.println(receivedData);
  }
}

void receiveEvent(int bytes) {
  byte index = 0;
  while  (Wire.available() && (index < 31)) {
    receivedData[index++] = Wire.read();
  }
}

void refreshServer() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Web started");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
//        Serial.write(c);
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println("Refresh: 10"); 
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print("Date: ");
          client.print(curDate);
          client.println("<br />");
          client.print("Time: ");
          client.print(curTime);
          client.println("<br />");
          client.print("Uptime: ");
          client.print(uptime);
          client.println("<br />");
          client.print("Temperature: ");
          client.print(temp);
          client.println("<br />");
          client.print("Humidity: ");
          client.print(humid);
          client.println("<br />");
          client.print("Heat: ");
          client.print(heat);
          client.println("<br />");
          client.print("Cooler: ");
          client.print(cooler);
          client.println("<br />");
          client.print("Humidifier: ");
          client.print(humidif);
          client.println("<br />");
          client.print("Light: ");
          client.print(light);
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
    Serial.println("Web disconnected");
  }
}

void receiver(char recieved[31]) {
  char* ptr = strtok(recieved, ";");
  int idx = 0;
    while(ptr != NULL && idx < 6) {
        printf("%s\n", ptr);
//        Serial.print("PTR: ");
//        Serial.println(ptr);
        switch (idx) {
          case 0:
            curDate = ptr; break;
          case 1:
            curTime = ptr; break;
          case 2:
            temp = String(ptr); break;
          case 3:
            humid = String(ptr); break;
          case 4:
            heat = handleSensors(String((String(ptr)).charAt(0)));
            cooler = handleSensors(String((String(ptr)).charAt(1)));
            humidif = handleSensors(String((String(ptr)).charAt(2)));
            light = handleSensors(String((String(ptr)).charAt(3))); break;
          case 5:
            uptime = String(ptr); break;
          default:
            break;
        }
        idx += 1;
        ptr = strtok(NULL, ";");
    }
}

String handleSensors(String data) {
  if (data == "1") { return "ON"; } else { return "OFF"; }
}

void sendToPushingBox(String message) {
  if (message.length() > 0) {
    EthernetClient client2;
    if (client2.connect("api.pushingbox.com", 80)) {
      Serial.println ("Connected to pushingbox");
            
      String postStr = "devid=";
      postStr += String(deviceId);
      postStr += "&message_param=";
      postStr += message;
      postStr += "\r\n\r\n";

      Serial.println("Sending..");
      client2.print("POST /pushingbox HTTP/1.1\n");
      client2.print("Host: api.pushingbox.com\n");
      client2.print("Connection: close\n");
      client2.print("Content-Type: application/x-www-form-urlencoded\n");
      client2.print("Content-Length: ");
      client2.print(postStr.length());
      client2.print("\n\n");
      client2.print(postStr);
      Serial.print("POST sent: ");
      Serial.println(postStr);
    }
    client2.stop();
  }
}
