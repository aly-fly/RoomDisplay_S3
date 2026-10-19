#include <Arduino.h>
#include <stdint.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include <utils.h>
#include "Clock.h"
#include "SD_Card.h"
#include "GlobalVariables.h"
#include "display.h"

/*
8.c:
https://urniki.easistent.com/urniki/izpis/12296fadef0fe622b9f04637b58002abc872259c/814563/0/0/0/4/9621355

2.m:
https://urniki.easistent.com/urniki/izpis/250363fbd15a5a588225b12e7bfb6f8b1a5d432f/802619/0/0/0/1/11026699
*/

const String eAsistent_T_URL1 = "https://urniki.easistent.com/urniki/izpis/12296fadef0fe622b9f04637b58002abc872259c/814563/0/0/0";
const String eAsistent_T_URL2 = "9621355";

const String eAsistent_M_URL1 = "https://urniki.easistent.com/urniki/izpis/250363fbd15a5a588225b12e7bfb6f8b1a5d432f/802619/0/0/0";
const String eAsistent_M_URL2 = "11026699";

const int FirstMondayInSchoolWeek_Day = 28;
const int FirstMondayInSchoolWeek_Month = 8;

unsigned long LastTimeUrnik1Refreshed = 0;
unsigned long LastTimeUrnik2Refreshed = 0;

#define numberOfHours (9+3)

// Class, day, hour
String Urnik[2][6][numberOfHours];

// declarations
void ProcessDataInit (void);
void ProcessData (int &urnikNr, String &DataIn);

void UrnikClear(int urnikNr) {
  for (int dan = 0; dan < 6; dan++) 
  {
    for (int ura = 0; ura < numberOfHours; ura++) 
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

  String URL;
  if (urnikNr == 0) { // T
    URL = eAsistent_T_URL1 + '/' + String(teden) + '/' + eAsistent_T_URL2;
  } else {            // M
    URL = eAsistent_M_URL1 + '/' + String(teden) + '/' + eAsistent_M_URL2;
  }
  DisplayText("Contacting: ", CLYELLOW);
  DisplayText(URL.c_str(), CLBLUE);
  DisplayText("\n");
  String sBufff;
  int NoMoreData;

  if (!loadFileFromSDcardToMerory("/cert/easistent-com.crt", Certificate, sizeof(Certificate), true))
  {
    return false;
  }

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
    #ifdef DEBUG_URNIK
    Serial.print("T = ");
    Serial.println(txt);
    #endif
    celica.concat(txt);
  }
  // text is now in the cell, do not use txt anymore from here down.

  section = DataIn.substring(delimiterPos+1, DataIn.length());  // to the end
  #ifdef DEBUG_URNIK
  Serial.print("S = ");
  Serial.println(section);
  #endif

  if (section.indexOf("Odpadla ura") >= 0) {
    odpadlaUra = true;
    #ifdef DEBUG_URNIK
    Serial.println("ODPADLA URA");
    #endif
    }

  if (section.indexOf("table")  == 0) {
    subTable = true; 
    subTableNum++;
    #ifdef DEBUG_URNIK_DETAIL
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
      } // odpadla ura
      celica.concat (" & ");
    }
  }

  if (section.indexOf("/table") == 0) {
    subTable = false;
    #ifdef DEBUG_URNIK_DETAIL
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
      #ifdef DEBUG_URNIK_DETAIL
      Serial.println("=== LINE END");
      #endif
    }
    if ((section.indexOf("/td") == 0) || (section.indexOf("/th") == 0)) { // konec celice --> vpis podatkov v urnik
      saveDan = dan;
      saveUra = ura;
      dataReady = true;
      subTableNum = 0;
      dan++;
      #ifdef DEBUG_URNIK_DETAIL
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

      // pobrisi nerelevanten del
      if ((celica.indexOf("Dekleta") > 0) && (urnikNr == 1)) {
        int pp = celica.indexOf('&');
        if (pp > 1) {
          celica.remove(pp-2);
          #ifdef DEBUG_URNIK
          Serial.println("Brisem del \"Dekleta\"");
          #endif
        }
      }        

      celica.trim(); // remove leading and trailing spaces
      TrimDoubleSpaces(celica);
      TrimNonPrintable(celica);
      #ifdef DEBUG_URNIK_DETAIL
      Serial.print("C ");
      Serial.print("[");
      Serial.print(saveDan);
      Serial.print("][");
      Serial.print(saveUra);
      Serial.print("] = ");
      Serial.println(celica);
      #endif

      if ((saveDan < 6) && (saveUra < numberOfHours)) {
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
  
  for (ura = 0; ura < numberOfHours; ura++) {
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

// ---------- date helpers (pure integer math, no timezone/DST issues) ----------

// Days since 1970-01-01 for a given civil date (m = 1..12)
static int32_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= (m <= 2);
  const int32_t  era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

// Weekday from day count: 0 = Monday ... 6 = Sunday (1970-01-01 was a Thursday)
static int weekdayFromDays(int32_t days) {
  return (int)(((days + 3) % 7 + 7) % 7);
}

// Day-of-month (1..7) of the first Monday in September of given year
static int firstMondayInSeptember(int year) {
  int wd = weekdayFromDays(daysFromCivil(year, 9, 1));  // weekday of 1. Sept
  return 1 + (7 - wd) % 7;
}

// ---------- school year info ----------

struct SchoolYearInfo {
  int startYear;     // calendar year in which this school year started
  int startDay;      // day in September of the first Monday (1..7)
  int dayNumber;     // day of school year, 1 = first Monday
  int week;          // school week, 1 = first week
  int weekday;       // 0 = Monday ... 6 = Sunday
};

static const char *dayNames[] = {"Monday", "Tuesday", "Wednesday", "Thursday",
                                  "Friday", "Saturday", "Sunday"};

// Returns false if the system time is not valid yet (e.g. NTP not synced)
bool getSchoolYearInfo(SchoolYearInfo &out) {
  time_t now;
  time(&now);
  struct tm t;
  localtime_r(&now, &t);
  if (t.tm_year + 1900 < 2024) return false;   // clock not set

  const int year = t.tm_year + 1900;
  const int32_t today = daysFromCivil(year, t.tm_mon + 1, t.tm_mday);

  // Start of the school year that began in this calendar year...
  int startYear = year;
  int startDay  = firstMondayInSeptember(startYear);
  int32_t start = daysFromCivil(startYear, 9, startDay);

  // ...if we haven't reached it yet, we're still in the school year
  // that started last autumn (this covers Jan..Aug and early Sept).
  if (today < start) {
    startYear--;
    startDay = firstMondayInSeptember(startYear);
    start    = daysFromCivil(startYear, 9, startDay);
  }

  // Serial.printf("First Monday in September: %04d-%02d-%02d\r\n", startYear, 9, startDay);

  const int32_t diff = today - start;          // days since first Monday
  out.startYear = startYear;
  out.startDay  = startDay;
  out.dayNumber = diff + 1;
  out.week      = diff / 7 + 1;
  out.weekday   = diff % 7;                    // start is a Monday, so this works
  return true;
}

//##################################################################################################################


void GetEAsistent(void) {
  Serial.println("GetEAsistent()");

  if (GetCurrentTime()) {
  } else {
    Serial.println("Error getting current time!");
  }

  // calculate week number
  int currentSchoolWeek;
  SchoolYearInfo syi;

  if (!getSchoolYearInfo(syi)) {
    Serial.println("Time not valid yet");
    return;
  }
  Serial.printf("School year start (first Monday in September): %04d-09-%02d\r\n", syi.startYear, syi.startDay);
  Serial.printf("Day of school year: %d\r\n", syi.dayNumber);
  Serial.printf("School week: %d\r\n", syi.week);
  Serial.printf("Weekday: %s (%d)\r\n", dayNames[syi.weekday], syi.weekday + 1);

  currentSchoolWeek = syi.week + 1; // starts with 1

  if (CurrentWeekday > 5) { // weekend
    currentSchoolWeek++; // show monday in the next week
    Serial.println("Weekend: week++");
  }
  if ((currentSchoolWeek < 1) || (currentSchoolWeek > 43)) {
    Serial.println("Error: wrong week number!");
    UrnikClear(0);
    UrnikClear(1);
    return;
  }

  Serial.print("Current week from school year start: ");
  Serial.println(currentSchoolWeek);

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
    tft.drawFastHLine(5, 5 + 21, 300, selectedColor);
    for (int i = 0; i < numberOfHours; i++)
    {
      if (Urnik[urnikNr][dayToShow][i].indexOf("ODP") >= 0) drawColor = CLRED; else
      if (i == 1) drawColor = CLORANGE; else
      if (i % 2) drawColor = CLWHITE; else
        drawColor = selectedColor;
      if (urnikNr == 0) font = FONT_URNIK_TT; else font = FONT_URNIK_TT;
      if (i == 1) Yshift = 6; // gap between name and first hour
      if (Urnik[urnikNr][dayToShow][i].length() < 1) // empty line
      {
        if ((!timeShown) && (i > 0)) 
        {
          DisplayText(Urnik[urnikNr][0][i].substring(7, Urnik[urnikNr][0][i].indexOf('-', 8)).c_str(), FONT_TXT_SMALL, 20, 5 + (i * 25) + Yshift + 3, CLCYAN, false);
          //timeShown = true;
        }
      }
      else // not empty line
      {
        DisplayText(Urnik[urnikNr][dayToShow][i].c_str(), font, 8, 5 + (i * 25) + Yshift, drawColor, false);
        timeShown = false;
      }
    }
  }
    delay(1500);
}
