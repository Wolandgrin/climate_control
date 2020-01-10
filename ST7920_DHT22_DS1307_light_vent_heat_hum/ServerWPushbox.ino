#include<Wire.h>
#include <SPI.h>
#include <Ethernet.h>

String deviceId = "vE88ABDC967551EF";
const char* logServer = "api.pushingbox.com";
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01 };
//IPAddress ip(192, 168, 1, 177); // IP address, may need to change depending on network
String data = "| 00 | 11 | 200 min | 23 | off | off | 70 | off | off |";
int temperature = 23;
int humidity = 95;

EthernetServer server(80);

void setup() {
  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    return;
  }
  delay(5000);
  Serial.print("Ethernet started with IP: ");
  Serial.println(Ethernet.localIP());
  server.begin();
}


void loop() {
refreshServer();
delay(1000);

  if (temperature <= 23){
    sendToPushingBox("Temperature is below minimum: " + String(temperature));
  } else if (temperature > 26) {
    sendToPushingBox("Temperature is above maximum: " + String(temperature));
  }
  
  if (humidity >= 95){
    sendToPushingBox("Humidity is above maximum: " + String(humidity));
  } else if (humidity < 65) {
    sendToPushingBox("Humidity is below minimum: " + String(humidity));
  }
delay(10000);
}

void refreshServer() {
   // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
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
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void sendToPushingBox(String message) {
  EthernetClient client;

  if (client.connect("api.pushingbox.com", 80)) {
    Serial.println ("Connected to server");
   
    String postStr = "devid=";
    postStr += String(deviceId);
    postStr += "&message_param=";
    postStr += String(message);
    postStr += "\r\n\r\n";
   
    Serial.print("Sending data");
   
    client.print("POST /pushingbox HTTP/1.1\n");
    client.print("Host: api.pushingbox.com\n");
    client.print("Connection: close\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    Serial.print("POST sent with message: ");
    Serial.println(postStr);  
  }
  client.stop();
  Serial.println("Stopping the client");

  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
    
}
