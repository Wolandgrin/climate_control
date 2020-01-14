#include<Wire.h>
#include <Ethernet.h>

String devId = "vE88ABDC967551EF";
const char* logServer = "api.pushingbox.com";
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01};
char receivedData[60]; 
String curDate;
String curTime;
String uptime;
long temp = 0;
long humid = 0;
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
  delay(1000);
  refreshServer();
  delay(5000);

  if (atol(temp) <= 22){
    sendToPushingBox("Temperature is below minimum: " + String(temp));
  } else if (atol(temp) > 27) {
    sendToPushingBox("Temperature is above maximum: " + String(temp));
  }
    delay(2000);

  if (atol(humid) >= 99){
    sendToPushingBox("Humidity is above maximum: " + String(humid));
  } else if (atol(humid) < 60) {
        sendToPushingBox("Humidity is below minimum: " + String(humid));
  }
  delay(2000);
}

void receiveEvent(int bytes) {
  byte index = 0;
  while  (Wire.available() && (index < 60)) {
    receivedData[index++] = Wire.read();
  }
  Serial.print("Received data: ");
  Serial.println(receivedData);
}

void refreshServer() {
  EthernetClient client = server.available();
  if (client) {
//    Serial.println("New client");
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

          client.print("Current date:  ");
          client.print(curDate);
          client.println("<br />");
          client.print("Current time:  ");
          client.print(curTime);
          client.println("<br />");
          client.print("System uptime: ");
          client.print(uptime);
          client.println("<br />");
          client.print("Temperature:   ");
          client.print(temp);
          client.println("<br />");
          client.print("Humidity:      ");
          client.print(humid);
          client.println("<br />");
          client.print("Heat:          ");
          client.print(heat);
          client.println("<br />");
          client.print("Cooler:        ");
          client.print(cooler);
          client.println("<br />");
          client.print("Humidifier:    ");
          client.print(humidif);
          client.println("<br />");
          client.print("Light:         ");
          client.print(light);
          client.println("<br />");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
//    Serial.println("Disconnected");
  }
}

void receiver(char recieved[60]) {
  char* ptr = strtok(recieved, ";");
  int idx = 0;
    while(ptr != NULL && idx < 6) {
        printf("%s\n", ptr);
        switch (idx) {
          case 0:
            curDate = ptr;
            break;
          case 1:
            curTime = ptr;
            break;
          case 2:
            uptime = String(ptr);
            break;
          case 3:
            temp = atol(ptr);
            break;
          case 4:
            humid = atol(ptr);
            break;
          case 5:
            heat = handleSensors(String((String(ptr)).charAt(0)));
            cooler = handleSensors(String((String(ptr)).charAt(1)));
            humidif = handleSensors(String((String(ptr)).charAt(2)));
            light = handleSensors(String((String(ptr)).charAt(3)));
            break;
          default:
            break;
        }
        idx += 1;
        ptr = strtok(NULL, ";");
    }
}

String handleSensors(String data) {
  if (data == "1") {
    return "ON";
  } else {
    return "OFF";
  }
}

void sendToPushingBox(String msg) {
  EthernetClient client2;
  if (client2.connect("api.pushingbox.com", 80)) {

    Serial.println("PB");
    String postStr = "devid=" + String(devId) + "&message_param=" + msg + "\r\n\r\n"; 
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
