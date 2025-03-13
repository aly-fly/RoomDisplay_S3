#include <Arduino.h>
#include <stdint.h>
#include <WiFi.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include "display.h"
#include "utils.h"
#include "Smoothie_TCP.h"

// reference: C:\Users\yyyyy\.platformio\packages\framework-arduinoespressif32\libraries\WiFi\examples\WiFiClientBasic\WiFiClientBasic.ino

// Use WiFiClient class to create TCP connections
WiFiClient clientSMO;
String Smoothie_TCPresponse;

bool Smoothie_TCPclientConnect(void) {
    Serial.println("TCPclientConnect()");
    bool result = false;
    if (!WiFi.isConnected()) {
        return false;
    }
    
    DisplayText("TCP socket: ");
    DisplayText(SMOOTHIE_HOST);
    DisplayText("\n");

    Serial.print("Connecting to ");
    Serial.println(SMOOTHIE_HOST);

    if (clientSMO.connect(SMOOTHIE_HOST, SMOOTHIE_PORT)) {
        Serial.println("Connected.");
        delay(100);
        if (!clientSMO.connected()) {
            DisplayText("CONN. DROPPED!\n", CLRED);
            Serial.println("... and connection dropped.");
            delay (1500);
            return false;
        }
        DisplayText("OK\n", CLGREEN);
        // check if server sent any welcome messages
        delay(300);
        String line = clientSMO.readString();
        Serial.print("TCP server connect message: ");
        Serial.println(line);
        result = true;
    } else {
        DisplayText("FAIL!\n", CLRED);
        delay(2000);
        Serial.println("Connection failed.");
        result = false;
    }
    delay (100);
    return result;
}

bool Smoothie_TCPclientRequest(const char Text[]) {
    Serial.println("TCPclientRequest()");
    if (!clientSMO.connected()) {
        //DisplayClear();
        if (!Smoothie_TCPclientConnect()) {
            return false;
        }
    }
    Serial.print("TCP request: ");
    Serial.println(Text);

    // This will send a request to the server
    clientSMO.println(Text);  // \n is required at the end

  int maxloops = 0;

  //wait for the server's reply to become available
  while (!clientSMO.available() && maxloops < 1000)
  {
    maxloops++;
    delay(1); //delay 1 msec
  }
  if (clientSMO.available() > 0)
  {
    //read back one line from the server
    Smoothie_TCPresponse = clientSMO.readStringUntil('\n');
    // clean the received data
    TrimNonPrintable(Smoothie_TCPresponse); 

    Serial.print("TCP reply: ");
    Serial.println(Smoothie_TCPresponse);
    //Smoothie_TCPclientDisconnect();
    return true;
  }
  else
  {
    Serial.println("TCP client timeout ");
    //Smoothie_TCPclientDisconnect();
    return false;
  }
}

void Smoothie_TCPclientDisconnect(void) {
    Serial.println("Smoothie_TCPclientDisconnect()");
    clientSMO.stop();
}
