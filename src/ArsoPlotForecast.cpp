#include <Arduino.h>
#include "display.h"
#include "ArsoXml.h"

void ArsoPlotForecast(void) {
    Serial.println("ArsoPlotForecast()");
    String Line;

    DisplayClear(CLWHITE);

    char FileName[30];
    sprintf(FileName, "/bg_sky_%dx%d.bmp", DspW, DspH);
    DisplayShowImage(FileName,   0, 0);

    // zgoraj - dnevi
    Line = ArsoWeather[0].DayName;
    DisplayText(Line.c_str(), 1,   10, 10, CLDARKBLUE);

    tft.setTextDatum(TR_DATUM); // top right
    Line = ArsoWeather[2].DayName;
    DisplayText(Line.c_str(), 1, DspW - 10, 10, CLDARKBLUE);

#ifdef IMAGES_ON_SD_CARD
  #define FOLDER  "/x2/"  // resized in advance in higher quality
#else
  #define FOLDER  "/w/"
#endif

    // na sredini so slikce (original 32x32, povečane x2)
    String FN;
    FN = FOLDER + ArsoWeather[0].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),   6, 44, 2);

    FN = FOLDER + ArsoWeather[1].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),  82, 44, 2);

    FN = FOLDER + ArsoWeather[2].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),  174, 44, 2);

    FN = FOLDER + ArsoWeather[3].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),  249, 44, 2);

    // wind data
    tft.setTextDatum(TL_DATUM); // top left (default)
    DisplayText(ArsoWeather[0].WindIcon.c_str(), 0,   6, 111, CLDARKBLUE);    
    DisplayText(ArsoWeather[1].WindIcon.c_str(), 0,  82, 111, CLDARKBLUE);    
    DisplayText(ArsoWeather[2].WindIcon.c_str(), 0, 174, 111, CLDARKBLUE);    
    DisplayText(ArsoWeather[3].WindIcon.c_str(), 0, 249, 111, CLDARKBLUE);    

    tft.setTextDatum(TR_DATUM); // top right
    DisplayText(ArsoWeather[0].Temperature.c_str(), 2,   85+2, 175+2, CLGREY); // shadow
    DisplayText(ArsoWeather[0].Temperature.c_str(), 2,   85,   175,   CLBLUE);
    DisplayText(ArsoWeather[1].Temperature.c_str(), 2,   85+2, 132+2, CLGREY); // shadow
    DisplayText(ArsoWeather[1].Temperature.c_str(), 2,   85,   132,   CLRED);
    DisplayText(ArsoWeather[2].Temperature.c_str(), 2,  260+2, 175+2, CLGREY); // shadow
    DisplayText(ArsoWeather[2].Temperature.c_str(), 2,  260,   175,   CLBLUE);
    DisplayText(ArsoWeather[3].Temperature.c_str(), 2,  260+2, 132+2, CLGREY); // shadow
    DisplayText(ArsoWeather[3].Temperature.c_str(), 2,  260,   132,   CLRED);

    tft.setTextDatum(TL_DATUM); // top left (default)
    DisplayText(SunRiseTime.c_str(), 1,    5,     DspH-23,   CLDARKGREEN);
    tft.setTextDatum(TR_DATUM); // top right
    DisplayText(SunSetTime.c_str(),  1, DspW-5,   DspH-23,   CLDARKGREEN);
    tft.setTextDatum(TL_DATUM); // top left (default)
    }

