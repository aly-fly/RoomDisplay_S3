#include <Arduino.h>
#include "display.h"
#include "ArsoXml.h"
#include "utils.h"

void ArsoPlotMeteogram(void) {
    Serial.println("ArsoPlotMeteogram()");
    DisplayClear(CLWHITE);
    float_t Xscaling = (float_t) DspW / (float_t)MTG_NUMPTS;
    Serial.printf("Scaling X: %f\r\n", Xscaling);

    float Yoffset = -14; // vertical shift data plot and numbers

    float_t Minn, Maxx;
    Minn =  999999999;
    Maxx = -999999999;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++) {
      if (ArsoMeteogram[i].TemperatureN > Maxx) {Maxx = ArsoMeteogram[i].TemperatureN;}
      if (ArsoMeteogram[i].TemperatureN < Minn) {Minn = ArsoMeteogram[i].TemperatureN;}
    }
    Serial.println("Min T: " + String(Minn));
    Serial.println("Max T: " + String(Maxx));

    Minn = Minn - 10;
    Maxx = Maxx + 10;
    
    float_t X1, X2, Y1, Y2, Yscaling;
    Yscaling = (float(DspH) / (Maxx - Minn));
    Serial.printf("Scaling Y: %f\r\n", Yscaling);
    if (Yscaling > 10) {
      Yscaling = 10; // limit Y zoom
      Yoffset =+ 45;
    }
    Serial.printf("Scaling Y limited: %f\r\n", Yscaling);

    int8_t MidnightIdx = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++) {
      if ((ArsoMeteogram[i].DayName.indexOf(" 1:00 ") > 0) ||
          (ArsoMeteogram[i].DayName.indexOf(" 2:00 ") > 0)) {
        MidnightIdx = i;
        Serial.print("Midnight idx = ");
        Serial.println(MidnightIdx);
        break;
      }
    }

    // vertical lines
    int32_t X;
    uint8_t idx;
    for (uint8_t i = 0; i < 4; i++) {
      idx = MidnightIdx + i * 8;
      X = round((Xscaling / 2) + ((idx) * Xscaling)) - 2;
      if (X >= DspW) {break;}

      int numDots = DspH / 6;
      for (int j = 0; j < numDots; j++)
      {
        tft.drawFastVLine(X, j*6, 3, TFT_DARKGREY);
      }
    }

    // plot rain & snow
    for (uint8_t i = 0; i < MTG_NUMPTS; i++) {
      X1 = (Xscaling / 2) + (i * Xscaling);

      Y1 = ArsoMeteogram[i].RainN * 10;
      Y2 = ArsoMeteogram[i].SnowN * 10;

      if ((Y1 + Y2) > DspH) {
        if (Y1 >= Y2) {
          Y1 = DspH - Y2; // trim the bigger one
        } else {
          Y2 = DspH - Y1; // trim the bigger one
        }
      }

      if (Y1 > 0) tft.fillRect(X1, DspH-Y1+1, 10, Y1, TFT_CYAN);
      if (Y2 > 0) tft.fillRect(X1, DspH-Y1-Y2, 10, Y2, TFT_PINK);
      delay(5);
    }

    // plot wind
    for (uint8_t i = 0; i < MTG_NUMPTS; i++) {
      X1 = (Xscaling / 2) + (i * Xscaling);

      Y1 = (ArsoMeteogram[i].WindN - 10) * 3;  // wind lower than 10 km/h is not plotted.

      if (Y1 > DspH) Y1 = DspH;

      if (Y1 > 0) tft.fillRect(X1+14, DspH-Y1, 2, Y1, TFT_GREEN);
      delay(5);
    }


    // day names:
    // 1. find out which day number and day name is today.
    uint8_t CurrDay;
    bool Found;
    String sToday = ArsoWeather[0].DayName;
    int8_t iToday = ArsoWeather[0].DayN;
    sToday.toUpperCase();
    for (uint8_t i = 0; i < 7; i++) {
      Found = sToday.indexOf(DAYS3[i]) > -1;
      if (Found) {
        CurrDay = i;
        Serial.print("Today is: ");
        Serial.print(iToday);
        Serial.print(", ");
        Serial.println(DAYS3[CurrDay]);
        break;
      }
    }

    // 2. Find on which index starts today in meteogram
    int TodayIdx = 0;
    for (uint8_t i = 0; i < MTG_NUMPTS; i++) {
      if (ArsoMeteogram[i].DayN == iToday) {
        TodayIdx = i;
        break;
      }
    }
    Serial.print("Index where Today starts = ");
    Serial.println(TodayIdx);

    // 3. Align Today with the midnight line
    int DayShift = 0;
    if (TodayIdx < MidnightIdx) DayShift = 1;

    // 4. plot day names
    int8_t DayIdx;
    for (uint8_t i = 0; i < 3; i++) {
      idx = MidnightIdx + i * 8;
      if (idx >= MTG_NUMPTS) {break;}

      X = round((Xscaling / 2) + (((float_t)idx) * Xscaling)) + 50;
      
      DayIdx = CurrDay + i + DayShift;
      if (DayIdx > 6) DayIdx -=7; // overflow
      if (DayIdx < 0) DayIdx +=7; // underflow
      DisplayText(DAYS3[DayIdx], FONT_TITLE, X, DspH-60, CLGREY);
    }




    // plot temperature graph
    for (uint8_t i = 1; i < MTG_NUMPTS; i++) {
      X1 = (Xscaling / 2) + ((i-1) * Xscaling);
      X2 = (Xscaling / 2) + ((i  ) * Xscaling);

      Y1 = (ArsoMeteogram[i-1].TemperatureN - Minn) * Yscaling + Yoffset;
      Y2 = (ArsoMeteogram[i  ].TemperatureN - Minn) * Yscaling + Yoffset;
      Y1 = DspH - Y1;
      Y2 = DspH - Y2;

      tft.drawWideLine(X1, Y1, X2, Y2, 3, CLORANGE, CLWHITE);
      delay(30);
    }

    // temperature low / blue
    tft.loadFont(FN_TEMP_METEO); // FONT_TEMP_METEO
    tft.setTextColor(CLBLUE, CLWHITE);
    for (uint8_t i = 0; i < 3; i++) {
      idx = MidnightIdx + i * 8 + 1;
      if (idx >= MTG_NUMPTS) {break;}
      X2 = (Xscaling / 2) + ((idx) * Xscaling);
      Y2 = (ArsoMeteogram[idx].TemperatureN - Minn) * Yscaling + Yoffset;
      Y2 = DspH - Y2;
      tft.drawNumber(round(ArsoMeteogram[idx].TemperatureN), X2, Y2 + 3, 2);
      delay(50);
    }

    // temperature high / red
    tft.setTextColor(CLRED, CLWHITE);
    for (uint8_t i = 0; i < 3; i++) {
      idx = MidnightIdx + i * 8 + 5;
      if (idx >= MTG_NUMPTS) {break;}
      X2 = (Xscaling / 2) + ((idx) * Xscaling);
      Y2 = (ArsoMeteogram[idx].TemperatureN - Minn) * Yscaling + Yoffset;
      Y2 = DspH - Y2;
      tft.drawNumber(round(ArsoMeteogram[idx].TemperatureN), X2, Y2 - 55, 2);
      delay(60);
    }

    // images
    String FN;

    for (uint8_t i = 0; i < MTG_NUMPTS; i++) {
      X2 = (i * Xscaling * 0.95);
      Y2 = 0;
      if ((i % 2) == 1) {Y2 = 32;}
      FN = "/w/" + ArsoMeteogram[i].WeatherIcon + ".bmp";
      DisplayShowImage(FN.c_str(),  round(X2), Y2);
    }

    // temperature high / red - AGAIN, write over images
    tft.setTextColor(CLRED, CLWHITE);
    for (uint8_t i = 0; i < 3; i++) {
      idx = MidnightIdx + i * 8 + 5;
      if (idx >= MTG_NUMPTS) {break;}
      X2 = (Xscaling / 2) + ((idx) * Xscaling);
      Y2 = (ArsoMeteogram[idx].TemperatureN - Minn) * Yscaling + Yoffset;
      Y2 = DspH - Y2;
      tft.drawNumber(round(ArsoMeteogram[idx].TemperatureN), X2, Y2 - 55, 2);
    }
    tft.unloadFont();
  }

