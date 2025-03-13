#include <Arduino.h>
#include <stdint.h>
#include <WiFi.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include "display.h"

// reference: C:\Users\yyyyy\.platformio\packages\framework-arduinoespressif32\libraries\WiFi\examples\WiFiClientBasic\WiFiClientBasic.ino

// Use WiFiClient class to create TCP connections
WiFiClient clientHP;
String HP_TCPresponse;

bool HP_TCPclientConnect(void) {
    Serial.println("TCPclientConnect()");
    bool result = false;
    if (!WiFi.isConnected()) {
        return false;
    }
    
    DisplayText("TCP socket: ");
    DisplayText(HEATPUMP_HOST);
    DisplayText("\n");

    Serial.print("Connecting to ");
    Serial.println(HEATPUMP_HOST);

    if (clientHP.connect(HEATPUMP_HOST, HEATPUMP_PORT)) {
        Serial.println("Connected.");
        delay(300);
        if (!clientHP.connected()) {
            DisplayText("CONN. DROPPED!\n", CLRED);
            Serial.println("... and connection dropped.");
            delay (1500);
            return false;
        }
        DisplayText("OK\n", CLGREEN);
        // check if server sent any welcome messages
        delay(300);
        String line = clientHP.readString();
        Serial.print("TCP server connect message: ");
        Serial.println(line);
        result = true;
    } else {
        DisplayText("FAIL!\n", CLRED);
        delay(2000);
        Serial.println("Connection failed.");
        result = false;
    }
    delay (1000);
    return result;
}

bool HP_TCPclientRequest(const char Text[]) {
    Serial.println("TCPclientRequest()");
    if (!clientHP.connected()) {
        //DisplayClear();
        if (!HP_TCPclientConnect()) {
            return false;
        }
    }
    Serial.print("TCP request: ");
    Serial.println(Text);

    // This will send a request to the server
    clientHP.println(Text);  // \n is required at the end

  int maxloops = 0;

  //wait for the server's reply to become available
  while (!clientHP.available() && maxloops < 1000)
  {
    maxloops++;
    delay(1); //delay 1 msec
  }
  if (clientHP.available() > 0)
  {
    //read back one line from the server
    HP_TCPresponse = clientHP.readStringUntil('\n');
    // clean the received data
    HP_TCPresponse.remove(HP_TCPresponse.indexOf(" "));

    Serial.print("TCP reply: ");
    Serial.println(HP_TCPresponse);
    return true;
  }
  else
  {
    Serial.println("TCP client timeout ");
    return false;
  }
}

void HP_TCPclientDisconnect(void) {
    Serial.println("Closing connection.");
    clientHP.stop();
}
