#include <Arduino.h>
#include "display.h"
#include "ArsoXml.h"

void ArsoPlotForecast(void) {
    Serial.println("ArsoPlotForecast()");
    String Line;

    DisplayClear(CLGREY);

    char FileName[30];
    sprintf(FileName, "/bg_sky_%dx%d.bmp", DspW, DspH);
    DisplayShowImage(FileName,   0, 0);

    // zgoraj - dnevi
    Line = ArsoWeather[0].DayName;
    DisplayText(Line.c_str(), FONT_TXT,   20, 15, CLDARKBLUE);

    tft.setTextDatum(TR_DATUM); // top right
    Line = ArsoWeather[2].DayName;
    DisplayText(Line.c_str(), FONT_TXT, DspW - 20, 15, CLDARKBLUE);

  #define FOLDER  "/x2/"  // resized in advance in higher quality

    // na sredini so slikce (original 32x32, povečane x2)
    String FN;
    FN = FOLDER + ArsoWeather[0].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),   10, 55, 2);

    FN = FOLDER + ArsoWeather[1].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),  10+90, 55, 2);

    FN = FOLDER + ArsoWeather[2].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),  10+90+200, 55, 2);

    FN = FOLDER + ArsoWeather[3].WeatherIcon + ".bmp";
    DisplayShowImage(FN.c_str(),  10+90+200+90, 55, 2);

    // wind data
    tft.setTextDatum(TL_DATUM); // top left (default)
    DisplayText(ArsoWeather[0].WindIcon.c_str(), FONT_TXT_SMALL, 10,           125, CLDARKBLUE);    
    DisplayText(ArsoWeather[1].WindIcon.c_str(), FONT_TXT_SMALL, 10+90,        125, CLDARKBLUE);    
    DisplayText(ArsoWeather[2].WindIcon.c_str(), FONT_TXT_SMALL, 10+90+200,    125, CLDARKBLUE);    
    DisplayText(ArsoWeather[3].WindIcon.c_str(), FONT_TXT_SMALL, 10+90+200+90, 125, CLDARKBLUE);    

    tft.setTextDatum(TR_DATUM); // top right
    DisplayText(ArsoWeather[0].Temperature.c_str(), FONT_TEMP_SINGLE,  40+90,          230,   CLBLUE);
    DisplayText(ArsoWeather[1].Temperature.c_str(), FONT_TEMP_SINGLE,  40+90,          155,   CLRED);
    DisplayText(ArsoWeather[2].Temperature.c_str(), FONT_TEMP_SINGLE,  40+90+200+90,   230,   CLBLUE);
    DisplayText(ArsoWeather[3].Temperature.c_str(), FONT_TEMP_SINGLE,  40+90+200+90,   155,   CLRED);

    tft.setTextDatum(TL_DATUM); // top left (default)
    DisplayText(SunRiseTime.c_str(), FONT_TXT,    5,     DspH-23,   CLDARKGREEN);
    tft.setTextDatum(TR_DATUM); // top right
    DisplayText(SunSetTime.c_str(),  FONT_TXT, DspW-5,   DspH-23,   CLDARKGREEN);
    tft.setTextDatum(TL_DATUM); // top left (default)
    }

