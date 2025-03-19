// ARSO xml: https://meteo.arso.gov.si/met/sl/service/
// osrednja 3 dni: https://meteo.arso.gov.si/uploads/probase/www/fproduct/text/sl/fcast_SLOVENIA_MIDDLE_latest.xml
// osrednja čez dan: https://meteo.arso.gov.si/uploads/probase/www/fproduct/text/sl/fcast_SI_OSREDNJESLOVENSKA_latest.xml
// trenutno stanje auto: https://meteo.arso.gov.si/uploads/probase/www/observ/surface/text/sl/observationAms_LJUBL-ANA_BRNIK_latest.xml
// trenutno stanje manu: https://meteo.arso.gov.si/uploads/probase/www/observ/surface/text/sl/observation_LJUBL-ANA_BRNIK_latest.xml

#include <Arduino.h>
#include <stdint.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include <utils.h>
#include "Clock.h"
#include "ArsoXml.h"
#include "SD_Card.h"
#include "GlobalVariables.h"
#include "display.h"

unsigned long LastTimeArsoRefreshed = 0; // data is not valid
unsigned long LastTimeArsoMeteogramRefreshed = 0; // data is not valid


// reference: "C:\Users\yyyyy\.platformio\packages\framework-arduinoespressif32\libraries\HTTPClient\examples\BasicHttpsClient\BasicHttpsClient.ino"

String XMLdata;

bool GetXmlDataFromServer(const char *URL) {
  bool result = false;
  
    if (!WiFi.isConnected()) {
        return false;
    }

  setClock();
  loadFileFromSDcardToMerory("/cert/meteo-arso-gov-si.crt", Certificate, sizeof(Certificate), true);

  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client->setHandshakeTimeout(10); // seconds (default 120 s)
    client->setTimeout(10);          // seconds (default  30 s)
    client->setCACert(Certificate);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
  
      Serial.print("[HTTPS] begin...\r\n");
      if (https.begin(*client, URL)) {  // HTTPS
        Serial.print("[HTTPS] GET...\r\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\r\n", httpCode);
  
          // file found at server
          if ((httpCode == HTTP_CODE_OK) || (httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            XMLdata = https.getString();
            result = XMLdata.length() > 1000;
            /*
            Serial.println("--- XML data begin ---");
            Serial.println(XMLdata);
            Serial.println("--- XML data end ---");
            */
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\r\n", https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\r\n");
      }

      // End extra scoping block
    }
  
    delete client;
  } else {
    Serial.println("Unable to create HTTPS client");
  }
    return result;
}


void TrimLogWords(String& Txt) {
  // trim long words
  if (Txt.length() > 3) {
    Txt.remove(3);   // 3 chars remaining
    Txt.concat('.');          
  }
}

ArsoWeather_t ArsoWeather[4];
ArsoWeather_t ArsoMeteogram[MTG_NUMPTS];
String SunRiseTime, SunSetTime;

// ###############################################################################################################################################

bool GetARSOdata(void) {
    Serial.println("GetARSOdata()");
    bool result = false;

    if ((millis() < (LastTimeArsoRefreshed + 60*60*1000)) && (LastTimeArsoRefreshed != 0)) {  // check server every hour
      Serial.println("ARSO data is valid.");
      return true;  // data is already valid
    }

    DisplayClear();

    Serial.println("Requesting current weather data from ARSO server...");
    DisplayText("Reading ARSO server current...\n", CLYELLOW);
    if (!GetXmlDataFromServer(ARSO_SERVER_CURRENT_XML_URL)) {
        XMLdata = "";  // free memory
        DisplayText("FAILED!\n", CLRED);
        delay(2000);
        return false;
    }

    Serial.println("[ARSO] Free memory: " + String(esp_get_free_heap_size()) + " bytes");

    DisplayText("OK\n", CLGREEN);
    DisplayText("Parsing data...\n");
    Serial.println("[ARSO] Parsing data...");
    // current  has index 0
    // forecast has index 1..3
    int ParamPos = 0;
    String S1;
    // Day number for today
    S1 =  utf8ascii(FindXMLParam(XMLdata, "valid", ParamPos).c_str());
    S1.remove(S1.indexOf("."));
    ArsoWeather[0].DayN = atoi(S1.c_str());

    ParamPos = 0;
    ArsoWeather[0].DayName = utf8ascii(FindXMLParam(XMLdata, "valid_day", ParamPos).c_str());
    // remove text after space
    ArsoWeather[0].DayName.remove(ArsoWeather[0].DayName.indexOf(" "));
    ArsoWeather[0].DayName.toUpperCase();

    ArsoWeather[0].PartOfDay = "trenutno"; // not existing in this xml

    ParamPos = 0;
    ArsoWeather[0].Rain = utf8ascii(FindXMLParam(XMLdata, "wwsyn_shortText", ParamPos).c_str());

    ParamPos = 0;
    ArsoWeather[0].Sky = utf8ascii(FindXMLParam(XMLdata, "nn_shortText", ParamPos).c_str());

    ParamPos = 0;
    ArsoWeather[0].Temperature = utf8ascii(FindXMLParam(XMLdata, "t_degreesC", ParamPos).c_str());

    ParamPos = 0;
    ArsoWeather[0].WeatherIcon = utf8ascii(FindXMLParam(XMLdata, "nn_icon-wwsyn_icon", ParamPos).c_str());

    ParamPos = 0;
    ArsoWeather[0].WindIcon = utf8ascii(FindXMLParam(XMLdata, "ddff_icon", ParamPos).c_str());


    ParamPos = 0;
    SunRiseTime = utf8ascii(FindXMLParam(XMLdata, "sunrise", ParamPos).c_str());
    SunSetTime  = utf8ascii(FindXMLParam(XMLdata, "sunset", ParamPos).c_str());
    XMLdata = "";  // free memory

    // <sunrise>27.03.2024 5:51 CET</sunrise>
    // <sunset>27.03.2024 18:25 CET</sunset>
    // isolate time data
    int p1, p2;
    p1 = SunRiseTime.indexOf(" ");
    p2 = SunRiseTime.indexOf(" ", p1+1);
    SunRiseTime.remove(p2);
    SunRiseTime.remove(0, p1);

    p1 = SunSetTime.indexOf(" ");
    p2 = SunSetTime.indexOf(" ", p1+1);
    SunSetTime.remove(p2);
    SunSetTime.remove(0, p1);




    Serial.println("Requesting forecast data from ARSO server...");
    DisplayText("Reading ARSO server forecast...\n", CLYELLOW);
    if (!GetXmlDataFromServer(ARSO_SERVER_FORECAST_XML_URL)) {
        XMLdata = "";  // free memory
        DisplayText("FAILED!\n", CLRED);
        delay(2000);
        return false;
    }

    Serial.println("[ARSO] Free memory: " + String(esp_get_free_heap_size()) + " bytes");

    DisplayText("OK\n", CLGREEN);
    DisplayText("Parsing data...\n");
    Serial.println("[ARSO] Parsing data...");
    // current  has index 0
    // forecast has index 1..3
    ParamPos = 0;
    for (uint8_t i = 1; i < 4; i++)
    {
        ArsoWeather[i].DayName = utf8ascii(FindXMLParam(XMLdata, "valid_day", ParamPos).c_str());
        // remove text after space
        ArsoWeather[i].DayName.remove(ArsoWeather[i].DayName.indexOf(" "));
//        TrimLogWords(ArsoWeather[i].Day);
        ArsoWeather[i].DayName.toUpperCase();
    }
    ParamPos = 0;
    for (uint8_t i = 1; i < 4; i++)
    {
        ArsoWeather[i].PartOfDay = utf8ascii(FindXMLParam(XMLdata, "valid_daypart", ParamPos).c_str());
        //TrimLogWords(ArsoWeather[i].PartOfDay);
    }
    ParamPos = 0;
    for (uint8_t i = 1; i < 4; i++)
    {
        ArsoWeather[i].Rain = utf8ascii(FindXMLParam(XMLdata, "wwsyn_shortText", ParamPos).c_str());
    }
    ParamPos = 0;
    for (uint8_t i = 1; i < 4; i++)
    {
        ArsoWeather[i].Sky = utf8ascii(FindXMLParam(XMLdata, "nn_shortText", ParamPos).c_str());
    }
    ParamPos = 0;
    for (uint8_t i = 1; i < 4; i++)
    {
        ArsoWeather[i].Temperature = utf8ascii(FindXMLParam(XMLdata, "t_degreesC", ParamPos).c_str());
    }
    ParamPos = 0;
    for (uint8_t i = 1; i < 4; i++)
    {
        ArsoWeather[i].WeatherIcon = utf8ascii(FindXMLParam(XMLdata, "nn_icon-wwsyn_icon", ParamPos).c_str());
    }
    ParamPos = 0;
    for (uint8_t i = 1; i < 4; i++)
    {
        ArsoWeather[i].WindIcon = utf8ascii(FindXMLParam(XMLdata, "ddff_icon", ParamPos).c_str());
    }
    XMLdata = "";  // free memory








    result = true;
    LastTimeArsoRefreshed = millis();

    for (uint8_t i = 0; i < 4; i++)
    {
      Serial.println("------------");
      Serial.println(ArsoWeather[i].DayName + "  " + ArsoWeather[i].PartOfDay + "  (" + ArsoWeather[i].DayN + ")");
      Serial.println(ArsoWeather[i].Sky);
      Serial.println(ArsoWeather[i].Rain);
      Serial.println(ArsoWeather[i].Temperature);
      Serial.println(ArsoWeather[i].WeatherIcon);
      Serial.println(ArsoWeather[i].WindIcon);
    }
    Serial.println("------------");
    Serial.print("Sunrise: ");
    Serial.println(SunRiseTime);
    Serial.print("Sunset: ");
    Serial.println(SunSetTime);
    Serial.println("------------");

    DisplayText("Finished\n");
    delay (500);
    return result;
}


// ###############################################################################################################################################

bool GetARSOmeteogram(void) {
    Serial.println("GetARSOmeteogram()");
    bool result = false;

    if ((millis() < (LastTimeArsoMeteogramRefreshed + 60*60*1000)) && (LastTimeArsoMeteogramRefreshed != 0)) {  // check server every hour
      Serial.println("ARSO meteogram data is valid.");
      return true;  // data is already valid
    }

    DisplayClear();

    Serial.println("Requesting meteogram data from ARSO server...");
    DisplayText("Reading ARSO server meteogram...\n", CLYELLOW);
    if (!GetXmlDataFromServer(ARSO_SERVER_METEOGRAM_XML_URL)) {
        XMLdata = "";  // free memory
        DisplayText("FAILED!\n", CLRED);
        delay(2000);
        return false;
    }

    Serial.println("[ARSO] Free memory: " + String(esp_get_free_heap_size()) + " bytes");

    DisplayText("OK\n", CLGREEN);
    DisplayText("Parsing data...\n");
    Serial.println("[ARSO] Parsing data...");

    String S1;

    // <valid>16.04.2024 2:00 CEST</valid>
    int ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        ArsoMeteogram[i].DayName = utf8ascii(FindXMLParam(XMLdata, "valid", ParamPos).c_str());
        S1 = ArsoMeteogram[i].DayName;
        S1.remove(S1.indexOf("."));
        ArsoMeteogram[i].DayN = atoi(S1.c_str());
    }

    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        ArsoMeteogram[i].PartOfDay = "";
    }

    // <wwsyn_shortText>dež s snegom</wwsyn_shortText>    
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        ArsoMeteogram[i].Rain = utf8ascii(FindXMLParam(XMLdata, "wwsyn_shortText", ParamPos).c_str());
    }

    // <nn_shortText>pretežno oblačno</nn_shortText>
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        ArsoMeteogram[i].Sky = utf8ascii(FindXMLParam(XMLdata, "nn_shortText", ParamPos).c_str());
    }

    // <nn_icon-wwsyn_icon>overcast_modSHRA</nn_icon-wwsyn_icon>
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        ArsoMeteogram[i].WeatherIcon = utf8ascii(FindXMLParam(XMLdata, "nn_icon-wwsyn_icon", ParamPos).c_str());
    }

    // <ddff_icon>lightNW</ddff_icon>
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        ArsoMeteogram[i].WindIcon = utf8ascii(FindXMLParam(XMLdata, "ddff_icon", ParamPos).c_str());
    }

    // <t>7.2</t>
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        S1 = utf8ascii(FindXMLParam(XMLdata, "t", ParamPos).c_str());
        ArsoMeteogram[i].TemperatureN = atof(S1.c_str());
    }

    // <tp_3h_acc>8.8</tp_3h_acc>
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        S1 = utf8ascii(FindXMLParam(XMLdata, "tp_3h_acc", ParamPos).c_str());
        ArsoMeteogram[i].RainN = atof(S1.c_str());
    }

    // <sn_3h_acc>0.4</sn_3h_acc>    
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        S1 = utf8ascii(FindXMLParam(XMLdata, "sn_3h_acc", ParamPos).c_str());
        ArsoMeteogram[i].SnowN = atof(S1.c_str());
    }

/*
<ff_val>4.9</ff_val>
<ffmax_val>11.3</ffmax_val>
<ff_val_kmh>18</ff_val_kmh>
<ffmax_val_kmh>41</ffmax_val_kmh>
*/
    ParamPos = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
        S1 = utf8ascii(FindXMLParam(XMLdata, "ffmax_val_kmh", ParamPos).c_str());
        ArsoMeteogram[i].WindN = atof(S1.c_str());
    }

    XMLdata = "";  // free memory








    result = true;
    LastTimeArsoMeteogramRefreshed = millis();

    for (uint8_t i = 0; i < MTG_NUMPTS; i++)
    {
      Serial.println("------------");
      Serial.println(ArsoMeteogram[i].DayName  + "  (" + ArsoMeteogram[i].DayN + ")");
      Serial.println(ArsoMeteogram[i].Sky);
      Serial.println(ArsoMeteogram[i].Rain);
      Serial.println(ArsoMeteogram[i].WeatherIcon);
      Serial.println(ArsoMeteogram[i].WindIcon);
      Serial.println(ArsoMeteogram[i].TemperatureN);
      Serial.println(ArsoMeteogram[i].RainN);
      Serial.println(ArsoMeteogram[i].SnowN);
      Serial.println(ArsoMeteogram[i].WindN);
    }
    Serial.println("------------");

    DisplayText("Finished\n");
    delay (500);
    return result;
}

void InvalidateArsoData(void) {
  LastTimeArsoRefreshed = 0;
  LastTimeArsoMeteogramRefreshed = 0;
}
