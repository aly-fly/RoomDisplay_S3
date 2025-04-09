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

// https://www.easistent.com/urniki/izpis/12296fadef0fe622b9f04637b58002abc872259c/600999/0/0/0/6/9421462

const String eAsistent_URL1 = "https://www.easistent.com/urniki/izpis/12296fadef0fe622b9f04637b58002abc872259c/";
const String eAsistent_URL2 = "/0/0/0/";

const int FirstMondayInSeptember = 2;

unsigned long LastTimeUrnik1Refreshed = 0;
unsigned long LastTimeUrnik2Refreshed = 0;

// Class, day, hour
String Urnik[2][6][10];

// declarations
void ProcessDataInit (void);
void ProcessData (int &urnikNr, String &DataIn);

void UrnikClear(int urnikNr) {
  for (int dan = 0; dan < 6; dan++) 
  {
    for (int ura = 0; ura < 10; ura++) 
    {
      Urnik[urnikNr][dan][ura].clear();
    }
    Urnik[urnikNr][dan][1] = "NO DATA";
  }
}

bool ReadEAsistentWebsite(int teden, int urnikNr) {
  Serial.println("ReadEAsistentWebsite()");
  bool Finished = false;
  if (!WiFi.isConnected()) {
      return false;
  }

  UrnikClear(urnikNr);
  
  setClock(); 

  DisplayClear();
  DisplayText("URNIK: ", CLPINK);
  DisplayText(String(urnikNr).c_str(), CLPINK);
  DisplayText("\n");

  String ucenec, razred;
  if (urnikNr == 0) {
    ucenec = "9621355";  // T
    razred = "600887";
  } else {
    ucenec = "9421462";  // M
    razred = "600999";
  }
  String URL = eAsistent_URL1 + razred + eAsistent_URL2 + '/' + String(teden) + '/' + ucenec;
  DisplayText("Contacting: ", CLYELLOW);
  DisplayText(URL.c_str(), CLBLUE);
  DisplayText("\n");
  String sBufff;
  int NoMoreData;

  loadFileFromSDcardToMerory("/cert/easistent-com.crt", Certificate, sizeof(Certificate), true);

  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client->setHandshakeTimeout(10); // seconds (default 120 s)
    client->setTimeout(10);          // seconds (default  30 s)
    client->setCACert(Certificate);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
  
      Serial.print("[HTTPS] begin...\r\n");
      DisplayText("HTTPS begin\n");
      if (https.begin(*client, URL)) {  // HTTPS
        yield(); // watchdog reset
        Serial.print("[HTTPS] GET...\r\n");
        DisplayText("HTTPS get request: ");
        // start connection and send HTTP header
        int httpCode = https.GET();
        yield(); // watchdog reset
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\r\n", httpCode);
          Serial.println(https.headerFirstLine());
          DisplayText(String(httpCode).c_str());
          DisplayText("\n");
          DisplayText(https.headerFirstLine().c_str());
          DisplayText("\n");

          // file found at server
          if (httpCode == HTTP_CODE_OK) {
                Serial.println("[HTTPS] Code OK");
                DisplayText("Code OK\n", CLGREEN);

// stream data in chunks
                // get length of document (is -1 when Server sends no Content-Length header)
                int DocumentLength = https.getSize();
                Serial.printf("[eAsistent] Document size: %d \r\n", DocumentLength);
                // get tcp stream
                WiFiClient * stream = https.getStreamPtr();
                // jump to the header
                Serial.println("Searching for the header...");
                DisplayText("Searching for the header...\n");
                bool HeaderFound = stream->find("<table");
                Serial.print("Header found: ");
                Serial.println(HeaderFound);
                if (HeaderFound) {
                  sBufff.clear();
                  Finished = false;
                  NoMoreData = 0;
                  Serial.println("Reading data from server...");
                  DisplayText("Reading data", CLYELLOW);
                  ProcessDataInit();
                  sBufff = stream->readStringUntil('>'); // first line is useless (class="ednevnik-seznam_ur_teden")
                  while ((!Finished)) {
                    sBufff = stream->readStringUntil('>');
                    DisplayText(".", CLGREY);
                    sBufff.replace("&nbsp;", " ");
                    sBufff.replace(TAB, SPACE);
                    sBufff.trim();
                    TrimDoubleSpaces(sBufff);
                    #ifdef DEBUG_OUTPUT_DATA
                    Serial.println("----------------");
                    Serial.println(sBufff);
                    Serial.println("----------------");
                    #endif

                    if (sBufff.indexOf("/body") >= 0) {
                      Finished = true;
                      Serial.println("End identifier found.");
                      DisplayText("\nEnd identifier found.\n", CLGREEN);
                    } // Finished
                    if (sBufff.length() > 0) {
                      ProcessData(urnikNr, sBufff);
                      NoMoreData = 0;
                    } else {  // No more data being received? 10 retries..
                      NoMoreData++;
                      delay(150);
                    }
                    if (NoMoreData > 10) break; // safety timeout
                  } // while
                  DisplayText("End of data\n", CLCYAN);
                  Serial.println("End of data");
                  if (!Finished) DisplayText("Timeout!", CLRED);
                } // header found
          } // HTTP code > 0
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\r\n", https.errorToString(httpCode).c_str());
          DisplayText("Error: ", CLRED);
          DisplayText(https.errorToString(httpCode).c_str(), CLRED);
          DisplayText("\n");
        }
  
        https.end();
      } else {
        Serial.println("[HTTPS] Unable to connect");
        DisplayText("Unable to connect.\n", CLRED);
      }

      // End extra scoping block
    }
  
    delete client;
  } else {
    Serial.println("Unable to create HTTPS client");
  }

  sBufff.clear(); // free mem
  if (Finished){
    Serial.println("Website read OK");
    DisplayText("Website read OK\n", CLGREEN);
  } else {
    Serial.println("Website read FAILED");
    DisplayText("Website read FAILED\n", CLRED);
    delay(3000);
  }
  return Finished;
}

//##################################################################################################################
//##################################################################################################################
//##################################################################################################################
//##################################################################################################################

// global
String celica;
int dan, ura;  // incremented values
bool subTable;
int subTableNum;

void ProcessDataInit (void) {
  dan = 0;
  ura = 0;
  subTableNum = 0;
  subTable = false;
  celica.clear();
}

void ProcessData (int &urnikNr, String &DataIn) {
  int delimiterPos;
  String section, txt, txt_utf;
  bool odpadlaUra, dataReady;
  int saveDan = 0, saveUra = 0;

  delimiterPos = 0;
  odpadlaUra = false;
  dataReady = false;

  txt.clear();
  section.clear();
  delimiterPos = DataIn.indexOf('<');
  if (delimiterPos < 0) return; // not found
  txt_utf = DataIn.substring(0, delimiterPos);
  txt = utf8ascii(txt_utf.c_str());

  txt.replace("Zgodovinski", "Zgo.");
  txt.replace("Geografski", "Geo.");
  txt.replace("krozek", "k.");
  txt.replace("Slovenscina", "SL");
  txt.replace("Anglescina", "AN");
  txt.replace("Matematika", "MT");
  txt.replace("Gospodinjstvo", "GS");
  txt.replace("Tehnika in tehnologija", "TT");
  txt.replace("Sport", "SP");

  txt.trim(); // remove leading and trailing spaces
  if (txt.length() > 0) {
    txt.concat(' '); // just one space at the end
    TrimDoubleSpaces(txt);
    #ifdef DEBUG_OUTPUT_DATA
    Serial.print("T = ");
    Serial.println(txt);
    #endif
    celica.concat(txt);
  }
  // text is now in the cell, do not use txt anymore from here down.

  section = DataIn.substring(delimiterPos+1, DataIn.length());  // to the end
  #ifdef DEBUG_OUTPUT_DATA
  Serial.print("S = ");
  Serial.println(section);
  #endif

  if (section.indexOf("ednevnik_seznam_ur_odpadlo") >= 0) {
    odpadlaUra = true;
    #ifdef DEBUG_URNIK
    Serial.println("ODPADLO");
    #endif
    }

  if (section.indexOf("table")  == 0) {
    subTable = true; 
    subTableNum++;
    #ifdef DEBUG_URNIK
    Serial.print("   Sub Table ");
    Serial.println(subTableNum);
    #endif
    if (subTableNum > 1) {
      // korigiraj odpadle ure
      if (odpadlaUra) {
        int pp = celica.lastIndexOf('&');
        if (pp >= 0) {  // naslednja sekcija iste celice
          celica.remove(pp+1);
          celica.concat(" ODPADLO");
        } else {
          celica = "ODPADLO ";
        }
        odpadlaUra = false;
      }
      celica.concat (" & ");
    }
  }

  if (section.indexOf("/table") == 0) {
    subTable = false;
    #ifdef DEBUG_URNIK
    Serial.println("   Sub Table end");
    #endif
    }

  if (subTable == false) { 
      if (section == "/tr") { // konec vrstice --> vpis podatkov v urnik
      /*
      saveDan = dan;
      saveUra = ura;
      dataReady = true;
      */
      subTableNum = 0;
      ura++;
      dan = 0;
      #ifdef DEBUG_URNIK
      Serial.println("=== LINE END");
      #endif
    }
    if ((section.indexOf("/td") == 0) || (section.indexOf("/th") == 0)) { // konec celice --> vpis podatkov v urnik
      saveDan = dan;
      saveUra = ura;
      dataReady = true;
      subTableNum = 0;
      dan++;
      #ifdef DEBUG_URNIK
      Serial.println("+++ CELL END");
      #endif
    }
  } // sub table

  if (dataReady) {
        // korigiraj odpadle ure
        if (odpadlaUra) {
          int pp = celica.lastIndexOf('&');
          if (pp >= 0) {  // naslednja sekcija iste celice
            celica.remove(pp+1);
            celica.concat(" ODPADLO");
          } else {
            celica = "ODPADLO ";
          }
          odpadlaUra = false;
        }
      celica.trim(); // remove leading and trailing spaces
      TrimDoubleSpaces(celica);
      TrimNonPrintable(celica);
      #ifdef DEBUG_URNIK
      Serial.print("C ");
      Serial.print("[");
      Serial.print(saveDan);
      Serial.print("][");
      Serial.print(saveUra);
      Serial.print("] = ");
      Serial.println(celica);
      #endif

      if ((saveDan < 6) && (saveUra < 10)) {
        Urnik[urnikNr][saveDan][saveUra] = celica;
        //Serial.println("Save."); 
      }
      celica.clear();
  }
}

//##################################################################################################################
//##################################################################################################################
//##################################################################################################################
//##################################################################################################################

void FinalizeData (int urnikNr, String sName) {
  int dan, ura;

  for (dan = 1; dan < 6; dan++) {
    Urnik[urnikNr][dan][0].concat(" " + sName);
  }

  #ifdef DEBUG_OUTPUT
  Serial.println("######################");
  
  for (ura = 0; ura < 10; ura++) {
    for (dan = 0; dan < 6; dan++) {
      Serial.print(Urnik[urnikNr][dan][ura]);
      Serial.print("  |  ");
    }
    Serial.println();
  }
  Serial.println("######################");
  #endif
}

//##################################################################################################################
//##################################################################################################################
//##################################################################################################################
//##################################################################################################################

/*
void eAsistentInit(void) {
  int urnikNr, dan, ura;  
  for (urnikNr = 0; urnikNr < 2; urnikNr++) 
    for (dan = 0; dan < 6; dan++) 
      for (ura = 0; ura < 10; ura++) 
        Urnik[urnikNr][dan][ura].reserve(25);
}
*/

void GetEAsistent(void) {
  Serial.println("GetEAsistent()");

  if (GetCurrentTime()) {
  } else {
    Serial.println("Error getting current time!");
  }

  // calculate week number
  int StartYear;
  struct tm sStartTime;
  time_t StartTime, CurrTime;
  time_t TdiffSec;
  int currentSchoolWeek;

  StartYear = CurrentYear;
  if (CurrentMonth < 8) { // after 1. Jan.
    StartYear--;
  }
  // 2.9.2024 = monday at 0:00
  // https://www.epochconverter.com/
  memset(&sStartTime, 0, sizeof(sStartTime));
  sStartTime.tm_year = StartYear - 1900;
  sStartTime.tm_mon = 9 - 1;
  sStartTime.tm_mday = FirstMondayInSeptember;
  sStartTime.tm_hour = 0;
  sStartTime.tm_min = 0;
  StartTime = mktime(&sStartTime);
  time(&CurrTime);
  TdiffSec = time_t(difftime(CurrTime, StartTime));
  Serial.printf("Current time: %d; Start time: %d\r\n", CurrTime, StartTime);
  Serial.print("Seconds elapsed from school year start: ");
  Serial.println(TdiffSec);
  currentSchoolWeek = TdiffSec / (7 * 24 * 60 * 60);
  currentSchoolWeek++; // starts with 1
  Serial.print("Current week from school year start: ");
  Serial.println(currentSchoolWeek);

  if (CurrentWeekday > 5) { // weekend
    currentSchoolWeek++; // show monday in the next week
    Serial.print("Weekend: week++");
  }
  if ((currentSchoolWeek < 1) || (currentSchoolWeek > 41)) {
    Serial.println("Error: wrong week number!");
    UrnikClear(0);
    UrnikClear(1);
    return;
  }

  if (! HasTimeElapsed(&LastTimeUrnik1Refreshed, 2*60*60*1000)) {  // check server every 2 hours
    Serial.println("Urnik 1: Data is valid.");
  } else {
    if (ReadEAsistentWebsite(currentSchoolWeek, 0)) {
      FinalizeData(0, "TINKARA");
    } else {
      LastTimeUrnik1Refreshed = 0; // retry      
    }
  }

  if (! HasTimeElapsed(&LastTimeUrnik2Refreshed, 2*60*60*1000)) {  // check server every 2 hours
    Serial.println("Urnik 2: Data is valid.");
  } else {
    if (ReadEAsistentWebsite(currentSchoolWeek, 1)) {
      FinalizeData(1, "MARCEL");
    } else {
      LastTimeUrnik2Refreshed = 0; // retry      
    }
  }
}








void DrawEAsistent(int urnikNr) {
  Serial.println("DrawEAsistent()");
  DisplayClear();
  if (GetCurrentTime()) {
    Serial.print("Day of the week = ");
    Serial.println(CurrentWeekday); // 1..7 = mon...sun
    Serial.print("Today = ");
    Serial.println(DAYSFULL[CurrentWeekday-1]);
    Serial.print("Hour = ");
    Serial.println(CurrentHour);
  } else {
    Serial.println("Error getting current time!");
  }
  int dayToShow = CurrentWeekday;

  if ((CurrentWeekday < 5) && (CurrentHour > 16)) { // mon...thu after 16:00
  // show next day
    dayToShow++;
    Serial.println("day++");
  }

  if (CurrentWeekday > 5) { // weekend
    dayToShow = 1; // monday
  }

  uint16_t selectedColor, drawColor;
  FontSize_t font;
  int16_t Yshift = 0;
  bool timeShown = false;
  if (urnikNr == 0) selectedColor = CLPINK; 
  if (urnikNr == 1) selectedColor = CLGREEN; 
  // process data for that day
  if (dayToShow > 0) {
    tft.drawFastHLine(5, 7 + 27, 300, selectedColor);
    for (int i = 0; i < 10; i++)
    {
      if (Urnik[urnikNr][dayToShow][i].indexOf("ODPADLO") >= 0) drawColor = CLRED; else
      if (i == 1) drawColor = CLORANGE; else
      if (i % 2) drawColor = CLWHITE; else
        drawColor = selectedColor;
      if (urnikNr == 0) font = FONT_URNIK_TT; else 
      font = FONT_URNIK_TT;
      if (i == 1) Yshift = 10; // gap between name and first hour
      if ((Urnik[urnikNr][dayToShow][i].length() < 1) && (!timeShown)) // empty line
      {
        DisplayText(Urnik[urnikNr][0][i].substring(6, Urnik[urnikNr][0][i].indexOf('-', 7)).c_str(), FONT_TXT_SMALL, 12, 7 + (i * 27) + Yshift + 3, CLCYAN, false);
        timeShown = true;
      }
      else
      {
        DisplayText(Urnik[urnikNr][dayToShow][i].c_str(), font, 8, 7 + (i * 27) + Yshift, drawColor, false);
        timeShown = false;
      }
    }
  }
    delay(1500);
}
