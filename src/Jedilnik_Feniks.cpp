#include <Arduino.h>
#include <stdint.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include <utils.h>
#include "Clock.h"
#include "Feniks_https_certificate.h"
#include "display.h"
#include "GlobalVariables.h"
#include "ArsoXml.h"  // current day

String JedilnikCeloten;
String JedilnikF[5];

unsigned long LastTimeFeniksRefreshed = 0; // data is not valid

const String Feniks_URL = "https://www.gostilnafeniks.si/";

bool ReadFeniksWebsite(void) {
  Serial.println("ReadFeniksWebsite()");
  bool result = false;
  bool Finished = false;
  unsigned int NoMoreData = 0;
  JedilnikCeloten.clear();

  if (!WiFi.isConnected()) {
      return false;
  }

  setClock(); 

  DisplayClear();
  DisplayText("Contacting: ");
  DisplayText(Feniks_URL.c_str());
  DisplayText("\n");
  String sBufOld;

  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client -> setCACert(rootCACertificate_Feniks);

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
  
      Serial.print("[HTTPS] begin...\r\n");
      DisplayText("HTTPS begin\n");
      if (https.begin(*client, Feniks_URL)) {  // HTTPS
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
          DisplayText(String(httpCode).c_str());
          DisplayText("\n");
  
          // file found at server
          if (httpCode == HTTP_CODE_OK) {
                Serial.println("[HTTPS] Streaming data from server in 3k byte chunks.");
                DisplayText("Reading data");

// stream data in chunks
                // get length of document (is -1 when Server sends no Content-Length header)
                int DocumentLength = https.getSize();
                Serial.printf("[Feniks] Document size: %d \r\n", DocumentLength);
                // get tcp stream
                WiFiClient * stream = https.getStreamPtr();

                // read all data from server
                while (https.connected() && (DocumentLength > 0 || DocumentLength == -1) && (!Finished)) {
                    yield(); // watchdog reset
                    // get available data size
                    size_t StreamAvailable = stream->available();
                    size_t BytesRead;

                    if (StreamAvailable) {
                        // read up to 3000 bytes
                        BytesRead = stream->readBytes(gBuffer, ((StreamAvailable > sizeof(gBuffer)) ? sizeof(gBuffer) : StreamAvailable));
                        Serial.print("/");
                        DisplayText(".");

                        if(DocumentLength > 0) {
                            DocumentLength -= BytesRead;
                        }
                        // process received data chunk
                        if (BytesRead > 0) {
                          // covert data to String
                          String sBuf;
                          sBuf = sBufOld;
                          sBuf.concat(String(gBuffer, BytesRead));
                          // glue last section of the previous buffer

                          int idxHeader = sBuf.indexOf("PONEDELJEK");
                          if (idxHeader >= 0) {
                            Serial.println();
                            Serial.print("Header found at ");
                            Serial.println(idxHeader);
                            DisplayText("\nHeader found.\n", CLGREEN);
                          }

                          int idxFooter = sBuf.indexOf("Tedenska ponudba");
                          if (idxFooter >= 0) {
                            Serial.print("Footer found at ");
                            Serial.println(idxFooter);
                            DisplayText("Footer found.\n", CLGREEN);
                          }

                          if ((idxHeader >= 0) && (idxFooter >= 0)) {
                            JedilnikCeloten = utf8ascii(sBuf.substring(idxHeader, idxFooter).c_str());
                            Finished = true;
                            break;
                            }
                          sBufOld = String(gBuffer, BytesRead);
                        } // process data (BytesRead > 0)
                    } // data available in stream
                    delay(20);
                    // No more data being received? 10 retries..
                    if (StreamAvailable == 0) {
                      NoMoreData++;
                      delay(150);
                      Serial.print("+");
                      //DisplayText("+");
                    }
                    else {
                      NoMoreData = 0;
                    }
                    if (Finished || (NoMoreData > 100)) { break; }
                } // connected or document still has data
                Serial.println();
                if (NoMoreData > 100) {
                  Serial.println("[HTTPS] Timeout.");
                  DisplayText("\nTimeout.\n", CLRED);
                  delay(2000);
                } else {
                  Serial.println("[HTTPS] Connection closed or file end.");
                }
// streaming end
            //DisplayText("\n");
            result = Finished;
          } // HTTP code > 0
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\r\n", https.errorToString(httpCode).c_str());
          DisplayText("Error: ");
          DisplayText(https.errorToString(httpCode).c_str());
          DisplayText("\n");
        }
  
        https.end();
      } else {
        Serial.println("[HTTPS] Unable to connect");
        DisplayText("Unable to connect.\n");
      }

      // End extra scoping block
    }
  
    delete client;
  } else {
    Serial.println("Unable to create HTTPS client");
  }

  sBufOld.clear(); // free mem
  if (result){
    Serial.println("Website read OK");
    DisplayText("Website read OK\n", CLGREEN);
  } else {
    Serial.println("Website read FAILED");
    DisplayText("Website read FAILED\n", CLRED);
    delay(2000);
  }
  return result;
}




void GetFeniks(void){
  Serial.println("GetFeniks()");

  if ((millis() < (LastTimeFeniksRefreshed + 2*60*60*1000)) && (LastTimeFeniksRefreshed != 0)) {  // check server every 2 hours
    Serial.println("Jedilnik Feniks: Data is valid.");
    return;  // data is already valid
  }

  if (ReadFeniksWebsite()) {
    LastTimeFeniksRefreshed = millis();

    /*
    Serial.println("==========================================");
    Serial.println(JedilnikCeloten);
    Serial.println("==========================================");
    */

    // parse data:

    // replace Newline with Space
    JedilnikCeloten.replace("\r", " ");
    JedilnikCeloten.replace("\n", " ");
    // remove visual formatting
    TrimNonPrintable(JedilnikCeloten);
    // replace </p> with CRLF
    JedilnikCeloten.replace("</p>", "\r\n");
    // replace space in the beginning of the line
    JedilnikCeloten.replace("\r ", "\r");

    // remove all sections "<...>"
    bool Found = true;
    int idx1 = 0, idx2 = 0;
    while (Found) {
      idx1 = JedilnikCeloten.indexOf('<', idx2);
      idx2 = JedilnikCeloten.indexOf('>', idx1+1);
      Found = ((idx1 >= 0) && (idx2 > idx1));
      if (Found) {
        int len = idx2-idx1+1;
        JedilnikCeloten.remove(idx1, len);
        idx2 = idx2 - len;
        }
    }  

    JedilnikCeloten.toLowerCase();
    JedilnikCeloten.trim(); // remove leading and trailing spaces
    TrimDoubleSpaces(JedilnikCeloten);
    Serial.println("==========================================");
    Serial.println(JedilnikCeloten);
    Serial.println("==========================================");

    idx1 = 0;
    idx2 = 0;
    // split to strings using day names
    for (int i = 0; i < 5; i++)
    {
      JedilnikF[i].clear();
      idx1 = JedilnikCeloten.indexOf(DAYSF[i]);
      idx2 = JedilnikCeloten.indexOf(DAYSF[i+1]);
      if (idx2 < 0) idx2 = JedilnikCeloten.length();
      JedilnikF[i] = JedilnikCeloten.substring(idx1, idx2);
    }
    JedilnikCeloten.clear(); // free mem
    // remove price for all lines
    for (int i = 0; i < 5; i++)
    {
      idx1 = JedilnikF[i].indexOf("0 e");
      while (idx1 > 4) {
        JedilnikF[i].remove(idx1 - 3, 7);
        idx1 = JedilnikF[i].indexOf("0 e");
      }
      idx1 = JedilnikF[i].indexOf("0e");
      while (idx1 > 4) {
        JedilnikF[i].remove(idx1 - 3, 6);
        idx1 = JedilnikF[i].indexOf("0e");
      }
      // remove year
      idx1 = JedilnikF[i].indexOf("202");
      if (idx1 > 0) {
        JedilnikF[i].remove(idx1, 4);
      }
    }

    // list extracted data
    Serial.println("-------------------");
      for (int i = 0; i < 5; i++)
      {
        Serial.println(JedilnikF[i]);
        Serial.println("-------------------");
      }
  }
}








void DrawFeniks(void) {
  Serial.println("DrawFeniks()");
  DisplayClear();
  String sToday = ArsoWeather[0].DayName;
  sToday.toLowerCase();
  sToday.remove(3);
  Serial.print("Today = ");
  Serial.println(sToday);


  int processSingleDay = -1;
  String DayF;
  // Monday .. Friday
  for (int day = 0; day < 5; day++) { // PON..PET
    DayF = DAYSF[day];
    DayF.remove(3);
    if (sToday.indexOf(DayF) == 0) 
    {
      // show next day
        if ((CurrentHour > 16) && (day < 4)) {
          day++;
          Serial.println("day++");
        }
      processSingleDay = day;
      break; // day matched
    }
  }

  // Sunday
  DayF = DAYSF[6];
  DayF.remove(3);  
  if (sToday.indexOf(DayF) == 0) {
    Serial.print("Today is Sunday");
    // show next day
      if (CurrentHour > 16) {
        processSingleDay = 0; // Monday
        Serial.print(" -> show Monday");
      }
      Serial.println();
  }

  // process data for that day
  if (processSingleDay > -1) {
      DisplayText("Feniks", FONT_TITLE, 200, 10, CLCYAN, true);
      DisplayText(JedilnikF[processSingleDay].c_str(), FONT_TXT, 40, 70, CLWHITE, true);
    } else { // weekend
      DisplayText("\n\n\n======================================\n", CLGREY);
      for (int i = 0; i < 3; i++) {
        DisplayText(JedilnikF[i].c_str(), CLYELLOW);
        DisplayText("\n======================================\n", CLGREY);
      }
  }
    delay(1500);
}

