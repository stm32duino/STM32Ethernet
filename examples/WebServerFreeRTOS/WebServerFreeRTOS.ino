/*
  Port of WebServer on FreeRTOS

  A simple web server that shows the value of the analog input pins.
  2 task are created:
    Ethernet: to manage the server
    Analog to read input values

  Circuit:
   STM32 board with Ethernet support
   Analog inputs attached to pins A0 through A5 (optional)

  created 18 Dec 2009
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe
  modified 02 Sept 2015
  by Arturo Guadalupi
  modified 23 Jun 2017
  by Wi6Labs
  modified 1 Jun 2018
  by sstaub
*/

#include <LwIP.h>
#include <STM32Ethernet.h>
#include <STM32FreeRTOS.h>

// Enter an IP address for your controller below.
// The IP address will be dependent on your local network:
IPAddress ip(192, 168, 0, 177);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

#define ANALOG_CHANEL_NUMBER 6
int sensorReading[ANALOG_CHANEL_NUMBER];

void taskAnalog(void* arg) {
  UNUSED(arg);
  while (1) {
    for (int analogChannel = 0; analogChannel < ANALOG_CHANEL_NUMBER; analogChannel++) {
      sensorReading[analogChannel] = analogRead(analogChannel);
    }
    vTaskDelay(1000); // read Analog every seconds
  }
}

// task code
void taskETH(void* arg) {
  UNUSED(arg);
  // start the Ethernet connection and the server:
  Ethernet.begin(ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  while (1) {
    // listen for incoming clients
    EthernetClient client = server.available();
    if (client) {
      Serial.println("new client");
      // an http request ends with a blank line
      bool currentLineIsBlank = true;
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
            for (int analogChannel = 0; analogChannel < ANALOG_CHANEL_NUMBER; analogChannel++) {
              client.print("analog input ");
              client.print(analogChannel);
              client.print(" is ");
              client.print(sensorReading[analogChannel]);
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
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  portBASE_TYPE s = xTaskCreate(taskETH, NULL, 200, NULL, 1, NULL);
  if (s != pdPASS) {
    printf("Ethernet task creation failed\n");
    while (1);
  }

  s = xTaskCreate(taskAnalog, NULL, 200, NULL, 2, NULL);
  if (s != pdPASS) {
    printf("Analog task creation failed\n");
    while (1);
  }

  vTaskStartScheduler();
  Serial.println("Scheduler failed");
  while (1);
}


void loop() {
  // Not used.
}