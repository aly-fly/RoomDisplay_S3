// https://meteo.arso.gov.si/uploads/probase/www/observ/radar/si0_zm_pda_anim.gif

#include <Arduino.h>
#include <stdint.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include <utils.h>
#include "Clock.h"
#include "SD_Card.h"
#include "GlobalVariables.h"
#include "display.h"
#include "GlobalVariables.h"
#include "GIFDraw.h"

unsigned long LastTimeArsoRainRefreshed = 0; // data is not valid

bool GetGIFimageFromServer(const char *URL)
{
    bool result = false;

    if (!WiFi.isConnected())
    {
        return false;
    }

    setClock();
    loadFileFromSDcardToMerory("/cert/meteo-arso-gov-si.crt", Certificate, sizeof(Certificate), true);

    GIFimageSize = 0;

    WiFiClientSecure *client = new WiFiClientSecure;
    if (client)
    {
        client->setHandshakeTimeout(10); // seconds (default 120 s)
        client->setTimeout(10);          // seconds (default  30 s)
        client->setCACert(Certificate);
        {
            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
            HTTPClient https;

            Serial.print("[HTTPS] begin...\r\n");
            if (https.begin(*client, URL))
            { // HTTPS
                Serial.print("[HTTPS] GET...\r\n");
                // start connection and send HTTP header
                int httpCode = https.GET();

                // httpCode will be negative on error
                if (httpCode > 0)
                {
                    // HTTP header has been send and Server response header has been handled
                    Serial.printf("[HTTPS] GET... code: %d\r\n", httpCode);
                    Serial.println(https.headerFirstLine());
                    DisplayText(String(httpCode).c_str());
                    DisplayText("\n");
                    DisplayText(https.headerFirstLine().c_str());
                    DisplayText("\n");
          
                    // file found at server
                    if ((httpCode == HTTP_CODE_OK) || (httpCode == HTTP_CODE_MOVED_PERMANENTLY))
                    {

                        WiFiClient *stream = https.getStreamPtr();
                        stream->setTimeout(2); // seconds
                        GIFimageSize = stream->readBytes(GIFimage, sizeof(GIFimage));
                        result = (GIFimageSize > 1000);
                        Serial.printf("GIF image size: %d bytes\r\n", GIFimageSize);
                    }
                }
                else
                {
                    Serial.printf("[HTTPS] GET... failed, error: %s\r\n", https.errorToString(httpCode).c_str());
                }

                https.end();
            }
            else
            {
                Serial.printf("[HTTPS] Unable to connect\r\n");
            }

            // End extra scoping block
        }

        delete client;
    }
    else
    {
        Serial.println("Unable to create HTTPS client");
    }
    return result;
}

// ###############################################################################################################################################

bool GetARSOrain(void)
{
    Serial.println("GetARSOrain()");
    bool result = false;

    if ((millis() < (LastTimeArsoRainRefreshed + 6 * 60 * 1000)) && (LastTimeArsoRainRefreshed != 0))
    { // check server every 6 minutes
        Serial.println("ARSO rain is valid.");
        return true; // data is already valid
    }

    DisplayClear();

    Serial.println("Requesting rain image from ARSO server...");
    DisplayText("Reading ARSO server rain image...\n", CLYELLOW);
    if (!GetGIFimageFromServer(ARSO_SERVER_RAIN_GIF_URL))
    {
        GIFimageSize = 0;
        DisplayText("FAILED!\n", CLRED);
        delay(2000);
        return false;
    }
    else
    {
        DisplayText("Image size: ", CLWHITE);
        DisplayText(String(GIFimageSize / 1000).c_str(), CLWHITE);
        DisplayText(" kB \nOK\n", CLGREEN);
        result = true;
    }

    LastTimeArsoRainRefreshed = millis();

    delay(500);
    return result;
}

// ###############################################################################################################################################
// ###############################################################################################################################################

void ShowARSOrainImage(void)
{
    Serial.println("DrawARSOrain()");
    if (GIFimageSize == 0)
    {
        Serial.println("Image size 0!");
        DisplayText("GIF: No image!", FONT_TITLE, 5, 20, CLRED);
        delay(2000);
        return;
    }

    DisplayClear(CLGREY);
    if (!DisplayGIF(GIFimage, GIFimageSize, 3))
    {
        DisplayText("GIF draw error!", FONT_TITLE, 5, 20, CLRED);
        delay(2000);
    }

    delay(50);
}

// ###############################################################################################################################################
// ###############################################################################################################################################

void InvalidateArsoRain(void)
{
    LastTimeArsoRainRefreshed = 0;
}
