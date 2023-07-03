/*
  WiFiAP_time_UDP.ino creates a WiFi access point and provides a web server on it.
  Code modified and combined from other projects by Dan Peirce B.Sc. July 2023
  modified from WiFiAccessPoint.ino
  * Created for arduino-esp32 on 04 July, 2018
      *by Elochukwu Ifediora (fedy0)
  Also modified AsyncUDPServer.ino
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ESP32Time.h>
#include "AsyncUDP.h"

//#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED

// Set these to your desired credentials.
const char *ssid = "proton0";
const char *password = "physics0";

WiFiServer server(80);
//ESP32Time rtc;
ESP32Time rtc(0);
struct tm tmstruct ;

AsyncUDP udp;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  // a valid password must have more than 7 characters
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
  rtc.setTime(1688382523+60); 
  
  delay(2000);


  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S")); 

  if(udp.listen(1234)) {
        Serial.print("UDP Listening on IP: ");
        Serial.println(WiFi.localIP());
        udp.onPacket([](AsyncUDPPacket packet) {
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
            //reply to the client
            //packet.printf("Got %u bytes of data", packet.length());
            packet.printf("%lu", rtc.getEpoch());
        });
    }
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<!DOCTYPE html><html><head><style>a:link, a:visited {background-color: #f44336;");
            client.print("color: white;padding: 4px 25px;text-align: center;text-decoration: none;display: inline-block;}");
            client.print("a:hover, a:active {background-color: blue;}</style></head><body>");
            client.print("Click <a href=\"/H\">here</a> to turn ON the LED. ");
            client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br><br>");
            client.print("\t<a href=\"/day+\">day+</a> ");
            client.print("\t<a href=\"/day-\">day-</a> ");
            client.print("\t<a href=\"/hour+\">hour+</a> ");
            client.print("\t<a href=\"/hour-\">hour-</a> ");
            client.print("<a href=\"/min+\">min+</a> ");
            client.print("<a href=\"/min-\">min-</a> ");
            client.print("<a href=\"/10min+\">10min+</a> ");
            client.print("<a href=\"/10min-\">10min-</a> <br><br>");
            client.print("\t\t<a href=\"/sec+\">sec+</a> ");
            client.print("\t\t<a href=\"/sec-\">sec-</a><br><br>");
            client.println(rtc.getEpoch());  
            client.print(rtc.getTime("%A, %B %d %Y %I:%M:%S ")); 
            client.println(rtc.getAmPm());
            client.println("</body></html>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
          Serial.println(rtc.getEpoch());  
          Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S")); 
        }
        if (currentLine.endsWith("GET /day+")) rtc.setTime(24*3600+rtc.getEpoch() );                // 
        if (currentLine.endsWith("GET /day-")) rtc.setTime(-24*3600+rtc.getEpoch() );
        if (currentLine.endsWith("GET /min+")) rtc.setTime(60+rtc.getEpoch() );
        if (currentLine.endsWith("GET /min-")) rtc.setTime(-60+rtc.getEpoch() );
        if (currentLine.endsWith("GET /10min+")) rtc.setTime(10*60+rtc.getEpoch() ); 
        if (currentLine.endsWith("GET /10min-")) rtc.setTime(-10*60+rtc.getEpoch() );
        if (currentLine.endsWith("GET /hour+")) rtc.setTime(3600+rtc.getEpoch());   
        if (currentLine.endsWith("GET /hour-")) rtc.setTime(-3600+rtc.getEpoch());  
        if (currentLine.endsWith("GET /sec+")) rtc.setTime(1+rtc.getEpoch()); 
        if (currentLine.endsWith("GET /sec-")) rtc.setTime(-1+rtc.getEpoch()); 
        if (currentLine.endsWith("GET /time"))udp.broadcastTo("Anyone here?", 1234);
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
