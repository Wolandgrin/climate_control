#include<Wire.h>
#include <Ethernet.h>

String deviceId = "vFEEC96DE2707EDF";// "vE88ABDC967551EF"; //or vFEEC96DE2707EDF
const char* logServer = "api.pushingbox.com";
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01};
char receivedData[33]; 
String curDate;
String curTime;
String uptime;
String temp;
String humid;
String heat;
String cooler;
String humidif;
String light; 

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
  delay(3000);
  refreshServer();
  delay(5000);
  handleNotifs();
  delay(2000);
}

void handleNotifs() {
  String msg;
  long int t = temp.toInt();
  long int h = humid.toInt();
//  Serial.println(t);
//  Serial.println(h);

  if (t <= 22){
    msg = "Temperature is below minimum: ";
    msg += String(temp);
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
  sendToPushingBox(msg);
}

void receiveEvent(int bytes) {
  byte index = 0;
  while  (Wire.available() && (index < 33)) {
    receivedData[index++] = Wire.read();
  }
  Serial.println(receivedData);
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
          lineBreak(client);
          client.print("Time: ");
          client.print(curTime);
          lineBreak(client);
          client.print("Uptime: ");
          client.print(uptime);
          lineBreak(client);
          client.print("Temperature: ");
          client.print(temp);
          lineBreak(client);
          client.print("Humidity: ");
          client.print(humid);
          lineBreak(client);
          client.print("Heat: ");
          client.print(heat);
          lineBreak(client);
          client.print("Cooler: ");
          client.print(cooler);
          lineBreak(client);
          client.print("Humidifier: ");
          client.print(humidif);
          lineBreak(client);
          client.print("Light: ");
          client.print(light);
          lineBreak(client);
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

void lineBreak(EthernetClient client) {
  client.println("<br />");
}

void receiver(char recieved[50]) {
  char* ptr = strtok(recieved, ";");
  int idx = 0;
    while(ptr != NULL && idx < 6) {
        printf("%s\n", ptr);
        switch (idx) {
          case 0:
            curDate = ptr; break;
          case 1:
            curTime = ptr; break;
          case 2:
            uptime = String(ptr); break;
          case 3:
            temp = String(ptr); break;
          case 4:
            humid = String(ptr); break;
          case 5:
            heat = handleSensors(String((String(ptr)).charAt(0)));
            cooler = handleSensors(String((String(ptr)).charAt(1)));
            humidif = handleSensors(String((String(ptr)).charAt(2)));
            light = handleSensors(String((String(ptr)).charAt(3))); break;
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
      Serial.println(message);
      
      String postStr = "devid=";
      postStr += String(deviceId);
      postStr += "&message_param=";
      postStr += message;
      postStr += "\r\n\r\n";

      Serial.print("Sending..");

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
