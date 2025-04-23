#include <Arduino.h>
#include <ArduinoOTA.h>
#include "display.h"


// hint: for web server version of OTA use: https://github.com/ayushsharma82/ElegantOTA

int divisor = 0;

//**************************************************************************************************
//                                         O T A S E T U P                                         *
//**************************************************************************************************
// Update via WiFi/Ethernet has been started by Arduino IDE or PlatformIO.                         *
//**************************************************************************************************

void OTA_init(void)
{
  Serial.println("OTA initialization...");
  ArduinoOTA.setRebootOnSuccess(true);

  ArduinoOTA.onStart([]()
                     {
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         Serial.println("OTA Update Started. Loading Program.");
                       }
                       else
                       {
                         Serial.println("OTA Update Started. Loading Data section.");
                       }

                       // stop any interrupts or background tasks here...
                       DisplayClear();
                       DisplayText("OTA start", FONT_TITLE, 5, 20, CLGREEN);
                       DisplayText(".", FONT_SYS, 5, 40, CLWHITE, true);

                       divisor = 99; // update LEDs imediatelly
                     });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
    divisor++;
    if (divisor >= 5)
    {
      unsigned int percent = (progress * 100) / total;
      DisplayText(".");
      divisor = 0;
    } });

  ArduinoOTA.onEnd([]()
                   {
    Serial.println("OTA Update Finished.");
    DisplayText("OTA FINISHED", FONT_TXT, 5, 100, CLGREEN);
    delay (600); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    const char* msgb = "" ;
    if ( error == OTA_AUTH_ERROR )
    {
      msgb = "Authentication Failed" ;
    }
    else if ( error == OTA_BEGIN_ERROR )
    {
      msgb = "Begin Failed" ;
    } 
    else if ( error == OTA_CONNECT_ERROR )
    {
      msgb = "Connection Failed" ;
    }
    else if ( error == OTA_RECEIVE_ERROR )
    {
      msgb = "Receive Failed" ;
    }
    else if ( error == OTA_END_ERROR )
    {
      msgb = "End Failed" ;
    }
    Serial.printf("OTA Error: %s\n", msgb);
    DisplayText("OTA ERROR", FONT_TXT, 5, 100, CLRED);
    DisplayText(msgb, FONT_TXT, 5, 120, CLRED);
    delay (2000); });

  ArduinoOTA.begin(); // Initialize
}

//**************************************************************************************************

void OTA_loop(void)
{
  ArduinoOTA.handle(); // Check for OTA
}

//**************************************************************************************************
