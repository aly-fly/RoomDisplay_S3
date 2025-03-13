
#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <SPIFFS.h>
#include "Version.h"
#include "__CONFIG.h"
#include "display.h"
#include "myWiFi.h"
#include "Clock.h"
#include "SD_Card.h"
#include "Heatpump_TCP.h"
#include "ArsoXml.h"
#include "ArsoPlotMeteogram.h"
#include "ArsoPlotForecast.h"
#include "ShellyHttpClient.h"
#include "CaptivePortalLogin.h"
#include "CoinCapAPI.h"
#include "myPing.h"
#include "Jedilnik_OS_Domzale.h"
#include "Jedilnik_Feniks.h"
#include "eAsistentUrnik.h"
#include "Smoothie_TCP.h"

int ScreenNumber = 0;
bool NightMode = false;
uint16_t LDRvalue;
String TempOutdoor1, TempOutdoor2;
bool ok;
String sCmd;
bool readAllData = false;

void setup() {
  Serial.begin(115200);
  pinMode(GPIO_NUM_0, INPUT_PULLUP);

  // delay 2 sec on the start to connect from programmer to serial terminal
  int i;
  for (i=0; i<10; i++){
    Serial.print("*");
    delay(100);
  }
  Serial.println();

  Serial.println("Project: github.com/aly-fly/RoomDisplay");
  Serial.print("Version: ");
  Serial.println(VERSION);
  Serial.print("Build: ");
  Serial.println(BUILD_TIMESTAMP);

#ifndef FREE_JTAG_PINS
  DisplayInit();
#endif  
  /*
  // TEST
  if (!digitalRead(GPIO_NUM_0)) {
      DisplayTest();
      DisplayFontTest();
  }
  */
  DisplayClear();

  DisplayText("Init...\n", CLYELLOW);
  DisplayText("Project: github.com/aly-fly/RoomDisplay_S3\n", CLWHITE);
  DisplayText("Version: ", CLWHITE);
  DisplayText(VERSION, CLCYAN);
  DisplayText("\n", CLWHITE);
  DisplayText("Build: ", CLWHITE);
  DisplayText(BUILD_TIMESTAMP, CLCYAN);
  DisplayText("\n", CLWHITE);

#ifdef LDR_PIN
  pinMode(LDR_PIN, ANALOG);
  adcAttachPin(LDR_PIN);
  analogSetAttenuation(ADC_0db);
#endif

  Serial.println("SPIFFS start...");
  DisplayText("SPIFFS start...");
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    DisplayText("FAILED!\n", CLRED);
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("SPIFFS available!");
  DisplayText("OK\n", CLGREEN);

#ifdef IMAGES_ON_SD_CARD
  Serial.println("SD Card start...");
  DisplayText("SD Card start...");
  if (!SDcardInit()) {
    Serial.println("SD Card initialisation failed!");
    DisplayText("FAILED!\n", CLRED);
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("SD Card available!");
  DisplayText("OK\n", CLGREEN);
  // TEST
  if (!digitalRead(GPIO_NUM_0)) {
    Serial.println("SD Card TEST!");
      SD_TEST();
  }
#endif

  WifiInit();

  if (!inHomeLAN) {
   bool connOk = CheckConnectivityAndHandleCaptivePortalLogin();
   if (!connOk) {
      Serial.println("=== REBOOT ===");
      DisplayText("=== REBOOT ===\n", CLORANGE);
      delay (30000);
      ESP.restart();  // retry everything from the beginning
   }
  }

  String sPingIP;
  IPAddress pingIP;
  sPingIP = "216.58.205.46"; // google.com
  pingIP.fromString(sPingIP);
  Serial.println(sPingIP);
  ppiinngg(pingIP, true);

  setClock(); 

#if DEVEL_JEDILNIK_OS != 0 // DEVEL mode
  ScreenNumber = 5;
#endif

  DisplayInitFonts();

  DisplayText("Init finished.", CLGREEN)    ;
  delay(2000);
  DisplayClear();
  Serial.println("INIT FINISHED.");
}


// ===============================================================================================================================================================

void loop() {
  // RX commands
  if (Serial.available() > 0) {
    sCmd.concat(Serial.readString()); // add new data to the existing queue
    int pp = sCmd.indexOf('\r');  // find first command
    if (pp > 0) {
      char Cmd = sCmd.charAt(pp-1);
      Serial.println("=====================================================");
      Serial.print("Command received: ");
      Serial.println(Cmd);
      switch (Cmd)
      {
      case 'A':
        Serial.println("-> Invalidate ARSO data");
        InvalidateArsoData();
        break;

      case 'B':
        Serial.println("-> Invalidate Bitcoin data");
        InvalidateCoinCapData();
        break;
      
      case 'J':
        Serial.println("-> Invalidate Jedilnik");
        InvalidateJedilnikOS();
        break;
      
      case 'N':  // "3N\r"
        ScreenNumber = sCmd.charAt(pp-2) - '0';
        Serial.print("-> Next screen = ");
        Serial.println(ScreenNumber);
        break;

      case 'D':
        Serial.println("-> Read additional data");
        readAllData = true;
        break;
      
      default:
        Serial.println("-> Unknown");
        break;
      }
      sCmd.clear(); // one at a time
    }
  } // serial available

  if(GetCurrentTime()) {
    Serial.println("Month: " + String(CurrentMonth));
    Serial.println("Day: " + String(CurrentDay));
    Serial.println("Hour: " + String(CurrentHour));
    NightMode = ((CurrentHour >= NIGHT_TIME) || (CurrentHour < DAY_TIME));
  } else {
    Serial.println("Getting current time failed!");
    NightMode = false;
  }

  // debug
  Serial.println("[IDLE] Free memory: " + String(esp_get_free_heap_size()) + " bytes");

  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); // internal RAM, memory capable to store data or to create new task
  /*
  info.total_free_bytes;   // total currently free in all non-continues blocks
  info.minimum_free_bytes;  // minimum free ever
  info.largest_free_block;   // largest continues block to allocate big array
  */
  Serial.println("[IDLE] Largest available block: " + String(info.largest_free_block) + " bytes");
  Serial.println("[IDLE] Minimum free ever: " + String(info.minimum_free_bytes) + " bytes");

  #ifdef LDR_PIN
  LDRvalue = analogRead(LDR_PIN); //     0 is full brightness, higher values == darker; Default is 12 bits (range from 0 to 4096).
  Serial.print("LDR: ");
  Serial.println(LDRvalue);

  DisplayText(String (LDRvalue).c_str());
  DisplayText("\n");

  delay(500);
  return;

  #else
  if (NightMode) {
    DisplaySetBrightness(NIGHT_TIME_BRIGHTNESS);
  } else { // lower brightness at night
    DisplaySetBrightness(); // full power
  }
  #endif

  //  HEAT PUMP DATA
  if (ScreenNumber == 0) {  // -------------------------------------------------------------------------------------------------------------------------
    if (!inHomeLAN) {
      ScreenNumber++;
    } else {
      DisplayClear(CLBLACK);
      char FileName[30];
      sprintf(FileName, "/bg_grass_%dx%d.bmp", DspW, DspH);
      DisplayShowImage(FileName,   0, 0);
      DisplayText("Temperatura pred hiso", FONT_TITLE,  45,   8, CLWHITE);
      DisplayText(SunRiseTime.c_str(), FONT_TXT,   5,          DspH-45-26-3,   CLYELLOW);
      DisplayText(SunSetTime.c_str(),  FONT_TXT,  DspW - 75,   DspH-45-26-3,   CLYELLOW);

      DisplayShowImage("/sunrise.bmp",  0,       DspH-45);
      DisplayShowImage("/sunset.bmp",   DspW-53, DspH-45);

      TempOutdoor1 = "- - -";
      if (HP_TCPclientRequest("Outdoor HP")) {
        HP_TCPresponse.replace(",", ".");
        // remove trailing "0"
        HP_TCPresponse.remove(HP_TCPresponse.indexOf(".")+2);
        HP_TCPresponse.concat(" C");
        TempOutdoor1 = HP_TCPresponse;
      }
      tft.setTextDatum(TR_DATUM); // top right
      DisplayText(TempOutdoor1.c_str(),    FONT_TEMP_SINGLE,  350,  70, CLORANGE);

      TempOutdoor2 = "- - -";
      if (HP_TCPclientRequest("Outdoor")) {
        HP_TCPresponse.replace(",", ".");
        // remove trailing "0"
        HP_TCPresponse.remove(HP_TCPresponse.indexOf(".")+2);
        HP_TCPresponse.concat(" C");
        TempOutdoor2 = HP_TCPresponse;
      }
      DisplayText(TempOutdoor2.c_str(),    FONT_TEMP_SINGLE,  350, 140, CLCYAN);

      char ShellyTxt[10];
      sprintf(ShellyTxt, "- - - kW");
      if (ShellyGetPower()) {
        sprintf(ShellyTxt, "%.2f kW", ShellyTotalPower/1000);
      }
      DisplayText(ShellyTxt,               FONT_TITLE, 300, 200, CLRED);

      tft.setTextDatum(TL_DATUM); // top left (default)

      // bazen
      if ((CurrentMonth >= 5) && (CurrentMonth <= 9)) {
        if (ShellyGetTemperature()) {}
        DisplayText(sShellyTemperature.c_str(), FONT_TITLE, 102, 230+2, CLBLACK); // shadow
        DisplayText(sShellyTemperature.c_str(), FONT_TITLE, 100, 230, CLLIGHTBLUE);

        uint32_t clr = CLDARKGREY;
        if (ShellyGetSwitch1()) {
          if (Shelly1ON) {clr = CLLIGHTBLUE;} else {clr = CLBLACK;}
        }
        tft.fillSmoothCircle(200, 230, 8, clr, CLDARKGREEN);
        clr = CLDARKGREY;
        if (ShellyGetSwitch2()) {
          if (Shelly2ON) {clr = CLORANGE;} else {clr = CLBLACK;}
          if (Shelly2Power > 100) {clr = CLRED;}
        }
        tft.fillSmoothCircle(230, 230, 8, clr, CLDARKGREEN);
      } // month
      delay(7000);
    }
  }

  // WEATHER FORECAST  
  if (ScreenNumber == 1) {  // -------------------------------------------------------------------------------------------------------------------------    
    ok = GetARSOdata();
    if (ok) ArsoPlotForecast();
    if (ok) delay(8000);
  }

  // Arso meteogram
  if (ScreenNumber == 2) {  // -------------------------------------------------------------------------------------------------------------------------
    ok = GetARSOmeteogram();
    if (ok) ArsoPlotMeteogram();
    if (ok) delay(13000);
  }

  // COIN CAP DATA PLOT
  if (ScreenNumber == 3) {  // -------------------------------------------------------------------------------------------------------------------------
    ok = GetCoinCapData_1H();
    PlotCoinCapData_1H();
    if (ok) delay(4000);
  }

  // COIN CAP DATA PLOT
  if (ScreenNumber == 4) {  // -------------------------------------------------------------------------------------------------------------------------
    ScreenNumber++;
   /*    
      if (!NightMode) {
      ok = GetCoinCapData_5M();
      PlotCoinCapData_5M();
      if (ok) delay(4000);
      } else ScreenNumber++;
   */      
  }

  // JEDILNIK FENIKS
  if (ScreenNumber == 5) {  // -------------------------------------------------------------------------------------------------------------------------
    GetFeniks();
    DrawFeniks();
    delay(13000);  
  }

  // JEDILNIK OŠ DOMŽALE
  if (ScreenNumber == 6) {  // -------------------------------------------------------------------------------------------------------------------------
    if (inHomeLAN || readAllData) {
      if ((CurrentMonth < 7) || (CurrentMonth > 8) || readAllData) {
        GetJedilnikOsDomzale();
        DrawJedilnikOsDomzale();
        delay(13000);
      }
    } else ScreenNumber++;
  }

  // URNIK OŠ DOMŽALE
  if (ScreenNumber == 7) {  // -------------------------------------------------------------------------------------------------------------------------
    if (inHomeLAN || readAllData) {
      if ((CurrentMonth < 7) || (CurrentMonth > 8) || readAllData) {
        GetEAsistent();
        DrawEAsistent(0);
        delay (7000);
        DrawEAsistent(1);
        delay (7000);
      }
    } else ScreenNumber++;
  }

  if (ScreenNumber == 8) {  // -------------------------------------------------------------------------------------------------------------------------
    if (!inHomeLAN) {
      ScreenNumber++;
    } else {
      if (Smoothie_TCPclientRequest("progress\r")) {  // CR line ending
        // Not currently playing
        // 88 % complete, elapsed time: 5273 s, est time: 687 s
        DisplayClear(CLBLACK);
        DisplayText("3D printer", FONT_TITLE, 70, 15, CLCYAN, false);
        DisplayText(Smoothie_TCPresponse.c_str(), FONT_TXT, 10, 70, CLYELLOW, true);
        String sStr;
        int Procent = 0;
        int Elapsed = 0;
        if (Smoothie_TCPresponse.indexOf('%') > 1) {
          sStr = Smoothie_TCPresponse.substring(0, Smoothie_TCPresponse.indexOf('%')-1);
          Procent = atoi(sStr.c_str());
          int xpix = (Procent * (tft.width()-10-8)) / 100;
          tft.drawRect(4  , tft.height()-46  , tft.width()-8 , 42  , CLWHITE);
          tft.drawRect(5  , tft.height()-45  , tft.width()-10, 40  , CLGREY);
          //tft.fillRect(5+4, tft.height()-45+4, xpix          , 40-8, CLGREEN);
          tft.fillRectHGradient(5+4, tft.height()-45+4, xpix          , 40-8, CLBLUE, CLGREEN);
        }
        if (Smoothie_TCPresponse.indexOf("elapsed") > 1) {
          sStr = Smoothie_TCPresponse.substring(Smoothie_TCPresponse.indexOf("elapsed")+14,
                                                Smoothie_TCPresponse.indexOf("s, est")-1);
          Elapsed = atoi(sStr.c_str());
          if (Procent > 5) {
            int Remaining = 0;
            Remaining = (Elapsed * (100 - Procent)) / Procent;
            Serial.print("Remaining (s): ");
            Serial.println(Remaining);
            int Hh = Remaining / 3600;
            int Mm = (Remaining - (Hh * 3600))/60;
            sStr = "Remaining: " + String(Hh) + "h " + String(Mm) + "m";
            DisplayText(sStr.c_str(), FONT_TITLE, 20, 170, CLBLUE);
          }
        }
        delay(10000);
      } // connect & request
    } // home lan
  }


  ScreenNumber++;
  if (ScreenNumber >= 9) { // housekeeping at the end of display cycles
    if (inHomeLAN)
      ScreenNumber = 0; else
      ScreenNumber = 1;   // skip heat pump

    WifiReconnectIfNeeded();

    // check connectivity
    if (!inHomeLAN) {
      //DisplayClear();
      //DisplayText("Test connectivity...\n", FONT_SYS, 0, 10, CLGREY);
      String sPingIP;
      IPAddress pingIP;
      sPingIP = "216.58.205.46"; // google.com
      pingIP.fromString(sPingIP);
      Serial.println(sPingIP);
      bool PingOK = ppiinngg(pingIP, false);
      if (!PingOK) {
        DisplayClear();
        DisplayText("Test connectivity FAILED\n", CLORANGE);

        bool connOk = CheckConnectivityAndHandleCaptivePortalLogin();
        if (!connOk){
            Serial.println("=== REBOOT ===");
            DisplayText("=== REBOOT ===\n", CLORANGE);
            delay (10000);
            ESP.restart();  // retry everything from the beginning
        }
      }
    }
  }

  delay(1000);
} // loop


