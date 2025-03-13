#include <Arduino.h>
#include <stdint.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include <utils.h>
#include "Clock.h"
#include "OsDomzale_https_certificate.h"
#include "display.h"
#include "Zamzar.h"
#include <FS.h>
#include <SPIFFS.h>
#include <SD.h>
#include "ArsoXml.h"

#define FileName "/jOsDom.txt"
#define DEVEL_FileNameDbg "/celJed.txt"
String Jedilnik[5];
String JedilnikDatum;

unsigned long LastTimeJedilnikRefreshed = 0; // data is not valid
bool ReloadRequired = false;

/*
 1. Load https://www.os-domzale.si/ (128 kB)
 2. Search for "<strong>Pomembne povezave</strong>" (located at ~53 kB; need ~600 bytes following this text)
 3. Search for "<li><a href="https://www.os-domzale.si/files/2024/04/jedilnik-2024-5-1.pdf">Jedilnik</a></li>"
 4. Extract URL of the PDF file
 5. Send to Zamzar cor conversion
 6. Download contents of the text file (4 kB)
 7. Get current day
 8. Analyze contents and extract data for today
*/


const String OSD_URL = "https://www.os-domzale.si/";
String PDF_URL, Saved_PDF_URL;


bool GetPdfLinkFromMainWebsite(void) {
  Serial.println("GetPdfLinkFromMainWebsite()");
  bool LinkFound = false;
  int safetyCounter = 0;
  PDF_URL = "";

  if (!WiFi.isConnected()) {
      return false;
  }

  setClock(); 

  DisplayText("Contacting: ");
  DisplayText(OSD_URL.c_str());
  DisplayText("\n");

  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client -> setCACert(rootCACertificate_OsDomzale);

    { // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
  
      Serial.print("[HTTPS] begin...\r\n");
      DisplayText("HTTPS begin\n");
      if (https.begin(*client, OSD_URL)) {  // HTTPS
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
            Serial.println("[HTTPS] Streaming data from server.");
            DisplayText("Reading data..\n");
            // get tcp stream
            WiFiClient * stream = https.getStreamPtr();
            bool HeaderFound = stream->find("Pomembne povezave");
            if (HeaderFound) {
              Serial.println("Header found.");
              DisplayText("Header found.\n", CLGREEN);
              String sBufff;
              sBufff = stream->readStringUntil('<'); // move cursor forward
              sBufff = "x"; // length > 0
              // scan all "<a href=...</a>" strings for the one containing "Jedilnik"
              // <li><a href="https://www.os-domzale.si/files/2024/04/jedilnik-2024-5-1.pdf">Jedilnik</a></li>
              Serial.println("Searching for the link");
              DisplayText("Searching for the link", CLYELLOW);
              while ((sBufff.length() > 0) && (!LinkFound)) {
                Serial.print('.');
                DisplayText(".");
                sBufff = stream->readStringUntil('<');
                if (sBufff.length() > 20) {
                  #ifdef DEBUG_OUTPUT_DATA
                    Serial.println(sBufff);
                  #endif
                  int idx1, idx2;
                  String aHref;
                  idx1 = sBufff.indexOf("a href=\"");
                  idx2 = sBufff.indexOf("\"", idx1+8);
                  #ifdef DEBUG_OUTPUT_DATA
                    Serial.println(idx1);
                    Serial.println(idx2);
                  #endif
                  if ((idx1 >= 0) && (idx2 > idx1) && (sBufff.indexOf("Jedilnik") > 0)) {
                    aHref = sBufff.substring(idx1+8, idx2);
                    Serial.print("\nLink found: ");
                    DisplayText("\nLink found: ", CLGREEN);
                    Serial.print(aHref);
                    DisplayText(aHref.c_str(), CLCYAN);
                    PDF_URL = aHref;
                    aHref.clear();
                    LinkFound = true;
                  } // Jedilnik
                } // buffer data available
                safetyCounter++;
                if (safetyCounter > 50) {
                  Serial.print("\nData not found in the first part!");
                  DisplayText("\nData not found in the first part!", CLRED);
                  delay(2000);
                }
              } // while data available and not found
              sBufff.clear(); // free mem
              Serial.println();
              DisplayText("\n");
            } // header found
          } // HTTP code OK
        } // HTTP code > 0
        else {
          Serial.printf("[HTTPS] GET... failed, error: %s\r\n", https.errorToString(httpCode).c_str());
          DisplayText("Error: ", CLRED);
          DisplayText(https.errorToString(httpCode).c_str(), CLRED);
          DisplayText("\n");
          delay(2000);
        }
        https.end();
      } // https begin
       else {
        Serial.println("[HTTPS] Unable to connect");
        DisplayText("Unable to connect.\n");
        delay(2000);
      }
    }  // End extra scoping block
  
    delete client;
  } else {
    Serial.println("Unable to create HTTPS client");
    delay(2000);
  }
  if (LinkFound){
    Serial.println(PDF_URL);
    DisplayText(PDF_URL.c_str(), CLCYAN);
    Serial.println("Website read OK");
    DisplayText("Website read OK\n", CLGREEN);
  } else {
    Serial.println("Website read & PDF search FAILED");
    DisplayText("Website read & PDF search FAILED\n", CLRED);
    delay(2000);
  }
  return LinkFound;
}



bool ReadSavedFile(void){
  Serial.println("ReadSavedFile()");
  DisplayText("Opening local file...");

  fs::File file = SPIFFS.open(FileName);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    DisplayText("Fail\n", CLRED);
    delay(2000);
    return false;
  }

  DisplayText(" Reading...");
  Serial.println("Reading from file");
  Saved_PDF_URL = file.readStringUntil(13);
  JedilnikDatum = file.readStringUntil(13);
  TrimNonPrintable(JedilnikDatum);
  for (int i = 0; i < 5; i++)
  {
    Jedilnik[i] = file.readStringUntil(13);
    TrimNonPrintable(Jedilnik[i]);
  }
  file.close();

  Serial.println("-------------------");
  Serial.println(Saved_PDF_URL);
  Serial.println("-------------------");
  Serial.println(JedilnikDatum);
  Serial.println("-------------------");
  for (int i = 0; i < 5; i++)
  {
    Serial.println(Jedilnik[i]);
  }
  Serial.println("-------------------");

  
  bool ok;
  ok = (Saved_PDF_URL.length() > 0) && (JedilnikDatum.length() > 0) && (Jedilnik[0].length() > 0) && (Jedilnik[1].length() > 0) && (Jedilnik[2].length() > 0) && (Jedilnik[3].length() > 0) && (Jedilnik[4].length() > 0);

  if (ok){
    Serial.println("File read OK");
    DisplayText("File read OK\n", CLGREEN);
  } else {
    Serial.println("File read FAIL");
    DisplayText("File read FAIL\n", CLRED);
    delay(2000);
  }

  return ok;  
}  


//#############################################################################################################################
//#############################################################################################################################
//#############################################################################################################################

#if DEVEL_JEDILNIK_OS != 2  // off or save

void GetJedilnikOsDomzale(void){
  Serial.println("GetJedilnikOsDomzale()");
  bool PdfOk, FileOk, NeedFreshData;

  if ((millis() < (LastTimeJedilnikRefreshed + 2*60*60*1000)) && (LastTimeJedilnikRefreshed != 0)) {  // check server every 2 hours
    Serial.println("Jedilnik OS: Data is valid.");
    return;  // data is already valid
  }


  DisplayClear();
  PdfOk = GetPdfLinkFromMainWebsite();
  FileOk = ReadSavedFile();
  NeedFreshData = false;

  if (PdfOk) LastTimeJedilnikRefreshed = millis();

  if (PdfOk && FileOk) {
    Serial.print("Comparing URLs.. ");
    DisplayText("Comparing URLs.. ");
    NeedFreshData = (Saved_PDF_URL != PDF_URL);

    if (NeedFreshData){
      Serial.println("NO match");
      DisplayText("NO match\n", CLORANGE);
    } else {
      Serial.println("match");
      DisplayText("match\n", CLGREEN);
    }
  }

  NeedFreshData = NeedFreshData || !FileOk || ReloadRequired;
  ReloadRequired = false;

  if (NeedFreshData && PdfOk) {
    if (ConvertPdfToTxt(PDF_URL)) {
      Serial.println("Conversion finished OK");
      DisplayText("Conversion finished OK\n", CLGREEN);
      //Serial.println(ZamzarData);

#if DEVEL_JEDILNIK_OS == 1 // save to file in SPIFFS

      Serial.println("Saving downloaded data into DEVEL file...");
      DisplayText("Saving into DEVEL file...");
      fs::File file1 = SPIFFS.open(DEVEL_FileNameDbg, FILE_WRITE);
      if(!file1){
          Serial.println("- failed to open file DEVEL for writing");
          DisplayText("FAIL\n", CLRED);
          delay(2000);
          return;
      }
      if(file1.print(ZamzarData)){
          Serial.println("- file written");
          DisplayText("OK\n", CLGREEN);
      } else {
          Serial.println("- write failed");
          DisplayText("FAIL\n", CLRED);
          delay(2000);
          file1.close();
          return;
      }
      file1.close();

#endif // DEVEL_JEDILNIK_OS == 1 // save to file

      String CelJedilnik = utf8ascii(ZamzarData.c_str());
      ZamzarData.clear(); // free mem
      //Serial.println(Jedilnik);

#ifdef SD_CS
      // Save txt to file on SD Card
      String TxtFileName = PDF_URL;
      int pp = TxtFileName.lastIndexOf('/');
      TxtFileName.remove(0, pp);
      TxtFileName.remove(TxtFileName.length()-3);
      TxtFileName.concat("txt");
      Serial.print("Saving converted text to file: ");
      Serial.println(TxtFileName);

      File fileTxt = SD.open(TxtFileName, FILE_WRITE);
      if (!fileTxt) {
        Serial.println("Failed to open file for writing");
        return;
      }
      if (fileTxt.print(CelJedilnik)) {
        Serial.println("File written");
      } else {
        Serial.println("Write failed");
      }
      fileTxt.close();
#endif

      JedilnikDatum.clear();
      int idxx = CelJedilnik.indexOf("202"); // 2024, 2025, ...
      if (idxx > 0) {
        JedilnikDatum = CelJedilnik.substring(idxx-15, idxx+4);
        JedilnikDatum.trim(); // remove leading and trailing spaces
        TrimDoubleSpaces(JedilnikDatum);
      }

      // remove footer
      CelJedilnik.remove(CelJedilnik.indexOf("Pridruzujemo"));
      // remove header
      int headerEnd;
      headerEnd = CelJedilnik.indexOf("PETEK");
      // other days too, of week is shorter
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("CETRTEK"); }
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("SREDA"); }
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("TOREK"); }
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("PONEDELJEK"); }

      CelJedilnik.remove(0, headerEnd + 6);
      Serial.println("===== File contents ==========================================");
      Serial.println(CelJedilnik);
      Serial.println("==============================================================");


// NEW ALGORYTHM START

      // fix extra capital words
      CelJedilnik.replace("SSSZ", "sssz");
      CelJedilnik.replace("Dodatno", "dodatno");
      CelJedilnik.replace(" EU ", " eu ");
      CelJedilnik.replace("Solske", "solske");

      // remove CR
      int p = CelJedilnik.indexOf(13);
      while (p >= 0)
      {
        CelJedilnik.remove(p, 1);
        p = CelJedilnik.indexOf(13);
      }
      // replace LF with '#' (this is a new line break symbol)
      p = CelJedilnik.indexOf(10);
      while (p >= 0)
      {
        CelJedilnik.setCharAt(p, '#');
        p = CelJedilnik.indexOf(10);
      }
      
      // remove empty lines
      TrimDoubleChars(CelJedilnik, '#');
      // remove first line break
      if (CelJedilnik.charAt(0) == '#') CelJedilnik.remove(0, 1);

      Serial.println("===== DEVEL 2 - fixed line endings ===========================");
      Serial.println(CelJedilnik);
      Serial.println("==============================================================");

      // buffer with separate lines
      String Line[25];
      int p1, p2, i, j;
      p1 = 0;
      p2 = CelJedilnik.indexOf('#');
      for (i = 0; i < 25; i++)
      {
        Line[i] = CelJedilnik.substring(p1, p2);
        p1 = p2+1;
        p2 = CelJedilnik.indexOf('#', p1);
        if (p2 == -1) { break; } // end of data        
      }
      CelJedilnik.clear();

      Serial.println("===== DEVEL 3 - separate lines ================================");
      for (i = 0; i < 25; i++) {
        Serial.print(i);
        Serial.print(": ");
        Serial.println(Line[i]);
      }
      Serial.println("==============================================================");

      int LongestLine = 0, Len;
      for (i = 0; i < 25; i++) {
        Len = Line[i].length();
        if (Len > LongestLine) { LongestLine = Len; }
      }
      Serial.print("Longest line = ");
      Serial.println(LongestLine);

      // make all lines same length - add spaces at the end
      for (i = 0; i < 25; i++) {
        Len = Line[i].length();
        for (j = 0; j < (LongestLine - Len); j++)
        {
          Line[i].concat(' ');
        }
      }

      Serial.println("===== DEVEL 4 - all lines same lenghth =======================");
      for (i = 0; i < 25; i++)
        Serial.println(Line[i]);
      Serial.println("==============================================================");

      // search for data in the first column (and ignore it)
      int namesPosition = 0;
      int ignoreFirstCols = 0;
      for (i = 0; i < 25; i++) {
        namesPosition = Line[i].indexOf("MALICA");
        if (namesPosition > 0) {
          ignoreFirstCols = namesPosition + 9;
          break;
        }
      }
      Serial.print("Ignore first columns: ");
      Serial.println(ignoreFirstCols);

      // search for empty columns & ignoring section on the left
      String dbgTxt1, dbgTxt2;
      Serial.print("Sections starting at: ");
      DisplayText("Sections: ");
      int idx, sectionNum;
      int colEndIdx[8];
      int colNumChars, prevColNumChars, colNumCapitals;
      for (i = 0; i < 8; i++)
      {
        colEndIdx[i] = 999; // if column is not found, this will prevent copying all data for remaining days
      }
      
      sectionNum = 0;
      prevColNumChars = 0;
      for (idx = ignoreFirstCols; idx < LongestLine; idx++)
      {
        colNumChars = 0;
        colNumCapitals = 0;
        for (i = 0; i < 25; i++)
        {
          if (Line[i].charAt(idx) != ' ') { colNumChars++; }
          if (IsUppercaseChar(Line[i].charAt(idx))) { colNumCapitals++; }
        }

        dbgTxt1.concat(String(colNumChars) + ';');
        dbgTxt2.concat(String(colNumCapitals) + ';');

        if ((prevColNumChars < 2) && (colNumCapitals > 0)) { // look for spaces followed by capital(s)
          Serial.print(idx);
          Serial.print(' ');
          DisplayText(String(idx).c_str());
          DisplayText(" ");
          colEndIdx[sectionNum] = idx;
          sectionNum++;
          if (sectionNum >= 8) {
            Serial.println("\nToo many sections found!");
            DisplayText("\nToo many sections!\n", CLRED);
            Serial.println(dbgTxt1);
            dbgTxt1.clear();
            Serial.println(dbgTxt2);
            dbgTxt2.clear();
            delay(2000);
            break;
          }
        }
        prevColNumChars = colNumChars;
      }
      DisplayText("\n");
      Serial.println();
      Serial.print("colNumChars: ");
      Serial.println(dbgTxt1);
      dbgTxt1.clear();
      Serial.print("colNumCapitals: ");
      Serial.println(dbgTxt2);
      dbgTxt2.clear();
      
      // copy data from columns to days

      for (int i = 0; i < 5; i++)
      {
        Jedilnik[i].clear();
      }

      for (i = 0; i < 25; i++) {
        Jedilnik[0].concat(Line[i].substring(colEndIdx[0], colEndIdx[1]));
        Jedilnik[1].concat(Line[i].substring(colEndIdx[1], colEndIdx[2]));
        Jedilnik[2].concat(Line[i].substring(colEndIdx[2], colEndIdx[3]));
        Jedilnik[3].concat(Line[i].substring(colEndIdx[3], colEndIdx[4]));
        Jedilnik[4].concat(Line[i].substring(colEndIdx[4])); // to the end
      }
      // free mem
      for (i = 0; i < 25; i++) {
        Line[i].clear();
      }

      Serial.println("===== DEVEL 5 - text in columns with spaces ==================");
      for (i = 0; i < 5; i++) {
        Serial.println(Jedilnik[i]);
        Serial.println("---------------");
      }
      Serial.println("==============================================================");

// NEW ALGORYTHM END

      // clean un-wanted words and chars
      for (int i = 0; i < 5; i++)
      {
        int idx = Jedilnik[i].indexOf("sssz:");
        if (idx > 0) Jedilnik[i].remove(idx, 5);
        idx = Jedilnik[i].indexOf("*");
        if (idx > 0) Jedilnik[i].remove(idx, 1);
        idx = Jedilnik[i].indexOf("1");
        if (idx > 0) Jedilnik[i].remove(idx, 1);
      }

      for (int i = 0; i < 5; i++)
      {
        TrimNonPrintable(Jedilnik[i]);
        Jedilnik[i].trim(); // remove leading and trailing spaces
        TrimDoubleSpaces(Jedilnik[i]);
        // if no data is found, add a sad face, so that file load works normally
        if (Jedilnik[i].length() == 0) {Jedilnik[i] = "(O_o)";}
      }
      // clean un-wanted words #2
      int p11, p22;
      for (int i = 0; i < 5; i++)
      {
        p11 = Jedilnik[i].indexOf("dodatno"); // Dodatno iz EU Solske sheme: / Dodatno iz EU sheme:
        if (p11 >=0) {
          p22 = Jedilnik[i].indexOf(':', p11);
          if (p22 > p11) {
            Jedilnik[i].remove(p11, p22-p11+2);
          }
        }
      }
      // list extracted data
      Serial.println("-------------------");
      Serial.println(PDF_URL);
      Serial.println("-------------------");
      Serial.println(JedilnikDatum);
      Serial.println("-------------------");
      for (int i = 0; i < 5; i++)
      {
        Serial.println(Jedilnik[i]);
      }
      Serial.println("-------------------");

      Serial.println("Saving data into local file...");
      DisplayText("Saving data into local file...");
      fs::File file = SPIFFS.open(FileName, FILE_WRITE);
      if(!file){
          Serial.println("- failed to open file for writing");
          DisplayText("FAIL\n", CLRED);
          delay(2000);
          return;
      }
      bool ok = true;
      if(file.println(PDF_URL)){
          Serial.println("- file written");
          DisplayText(".");
      } else {
          Serial.println("- write failed");
          ok = false;         
      }
      if(file.println(JedilnikDatum)){
          Serial.println("- file written");
          DisplayText(".");
      } else {
          Serial.println("- write failed");
          ok = false;         
      }

      for (int i = 0; i < 5; i++)
      {
        if(file.println(Jedilnik[i])){
            Serial.println("- file written");
            DisplayText(".");
        } else {
            Serial.println("- write failed");
            ok = false;         
        }
      }
      file.close();
      if(ok) {
          Serial.println("File write OK");
          DisplayText("OK\n", CLGREEN);
      } else {
          Serial.println("File write FAIL");
          DisplayText("FAIL\n", CLRED);
          delay(2000);
          return;
      }

    } else { // ConvertPdfToTxt OK
      return;
    }
  } // (NeedFreshData && PdfOk)
}

#endif // DEVEL_JEDILNIK_OS != 2  // off or load&test

//#############################################################################################################################
//#############################################################################################################################
//#############################################################################################################################

#if DEVEL_JEDILNIK_OS == 2  // load & test

void GetJedilnikOsDomzale(void){
  Serial.println("GetJedilnikOsDomzale() DEVEL");



  DisplayClear();
  if (true) {
    if (true) {

      if (!SPIFFS.exists(DEVEL_FileNameDbg)) {
        Serial.println("File doesn't exist!");
        DisplayText("File doesn't exist!\n", CLRED);
        delay(2000);        
        return;
      }

      fs::File file1 = SPIFFS.open(DEVEL_FileNameDbg, FILE_READ);
      if(!file1){
        Serial.println("- failed to open file DEVEL for reading");
        DisplayText("Fail DEVEL file open\n", CLRED);
        delay(2000);
        return;
      }

      DisplayText(" Reading...");
      Serial.println("Reading from file DEVEL");
      ZamzarData = file1.readString();
      file1.close();


      String CelJedilnik = utf8ascii(ZamzarData.c_str());
      ZamzarData.clear(); // free mem
      //Serial.println(Jedilnik);

      JedilnikDatum.clear();
      int idxx = CelJedilnik.indexOf("202"); // 2024, 2025, ...
      if (idxx > 0) {
        JedilnikDatum = CelJedilnik.substring(idxx-15, idxx+4);
        JedilnikDatum.trim(); // remove leading and trailing spaces
        TrimDoubleSpaces(JedilnikDatum);
      }

      // remove footer
      CelJedilnik.remove(CelJedilnik.indexOf("Pridruzujemo"));
      // remove header
      int headerEnd;
      headerEnd = CelJedilnik.indexOf("PETEK");
      // other days too, of week is shorter
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("CETRTEK"); }
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("SREDA"); }
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("TOREK"); }
      if (headerEnd < 0) { headerEnd = CelJedilnik.indexOf("PONEDELJEK"); }

      CelJedilnik.remove(0, headerEnd + 6);
      Serial.println("===== File contents ==========================================");
      Serial.println(CelJedilnik);
      Serial.println("==============================================================");


// NEW ALGORYTHM START

      // fix extra capital words
      CelJedilnik.replace("SSSZ", "sssz");
      CelJedilnik.replace("Dodatno", "dodatno");
      CelJedilnik.replace(" EU ", " eu ");
      CelJedilnik.replace("Solske", "solske");

      // remove CR
      int p = CelJedilnik.indexOf(13);
      while (p >= 0)
      {
        CelJedilnik.remove(p, 1);
        p = CelJedilnik.indexOf(13);
      }
      // replace LF with '#' (this is a new line break symbol)
      p = CelJedilnik.indexOf(10);
      while (p >= 0)
      {
        CelJedilnik.setCharAt(p, '#');
        p = CelJedilnik.indexOf(10);
      }
      
      // remove empty lines
      TrimDoubleChars(CelJedilnik, '#');
      // remove first line break
      if (CelJedilnik.charAt(0) == '#') CelJedilnik.remove(0, 1);

      Serial.println("===== DEVEL 2 - fixed line endings ===========================");
      Serial.println(CelJedilnik);
      Serial.println("==============================================================");

      // buffer with separate lines
      String Line[25];
      int p1, p2, i, j;
      p1 = 0;
      p2 = CelJedilnik.indexOf('#');
      for (i = 0; i < 25; i++)
      {
        Line[i] = CelJedilnik.substring(p1, p2);
        p1 = p2+1;
        p2 = CelJedilnik.indexOf('#', p1);
        if (p2 == -1) { break; } // end of data        
      }
      CelJedilnik.clear();

      Serial.println("===== DEVEL 3 - separate lines ================================");
      for (i = 0; i < 25; i++) {
        Serial.print(i);
        Serial.print(": ");
        Serial.println(Line[i]);
      }
      Serial.println("==============================================================");

      int LongestLine = 0, Len;
      for (i = 0; i < 25; i++) {
        Len = Line[i].length();
        if (Len > LongestLine) { LongestLine = Len; }
      }
      Serial.print("Longest line = ");
      Serial.println(LongestLine);

      // make all lines same length - add spaces at the end
      for (i = 0; i < 25; i++) {
        Len = Line[i].length();
        for (j = 0; j < (LongestLine - Len); j++)
        {
          Line[i].concat(' ');
        }
      }

      Serial.println("===== DEVEL 4 - all lines same lenghth =======================");
      for (i = 0; i < 25; i++)
        Serial.println(Line[i]);
      Serial.println("==============================================================");

      // search for data in the first column (and ignore it)
      int namesPosition = 0;
      int ignoreFirstCols = 0;
      for (i = 0; i < 25; i++) {
        namesPosition = Line[i].indexOf("MALICA");
        if (namesPosition > 0) {
          ignoreFirstCols = namesPosition + 9;
          break;
        }
      }
      Serial.print("Ignore first columns: ");
      Serial.println(ignoreFirstCols);

      // search for empty columns & ignoring section on the left
      String dbgTxt1, dbgTxt2;
      Serial.print("Sections starting at: ");
      DisplayText("Sections: ");
      int idx, sectionNum;
      int colEndIdx[8];
      int colNumChars, prevColNumChars, colNumCapitals;
      for (i = 0; i < 8; i++)
      {
        colEndIdx[i] = 999; // if column is not found, this will prevent copying all data for remaining days
      }
      
      sectionNum = 0;
      prevColNumChars = 0;
      for (idx = ignoreFirstCols; idx < LongestLine; idx++)
      {
        colNumChars = 0;
        colNumCapitals = 0;
        for (i = 0; i < 25; i++)
        {
          if (Line[i].charAt(idx) != ' ') { colNumChars++; }
          if (IsUppercaseChar(Line[i].charAt(idx))) { colNumCapitals++; }
        }

        dbgTxt1.concat(String(colNumChars) + ';');
        dbgTxt2.concat(String(colNumCapitals) + ';');

        if ((prevColNumChars < 2) && (colNumCapitals > 0)) { // look for spaces followed by capital(s)
          Serial.print(idx);
          Serial.print(' ');
          DisplayText(String(idx).c_str());
          DisplayText(" ");
          colEndIdx[sectionNum] = idx;
          sectionNum++;
          if (sectionNum >= 8) {
            Serial.println("\nToo many sections found!");
            DisplayText("\nToo many sections!\n", CLRED);
            Serial.println(dbgTxt1);
            dbgTxt1.clear();
            Serial.println(dbgTxt2);
            dbgTxt2.clear();
            delay(2000);
            break;
          }
        }
        prevColNumChars = colNumChars;
      }
      DisplayText("\n");
      Serial.println();
      Serial.print("colNumChars: ");
      Serial.println(dbgTxt1);
      dbgTxt1.clear();
      Serial.print("colNumCapitals: ");
      Serial.println(dbgTxt2);
      dbgTxt2.clear();
      
      // copy data from columns to days

      for (int i = 0; i < 5; i++)
      {
        Jedilnik[i].clear();
      }

      for (i = 0; i < 25; i++) {
        Jedilnik[0].concat(Line[i].substring(colEndIdx[0], colEndIdx[1]));
        Jedilnik[1].concat(Line[i].substring(colEndIdx[1], colEndIdx[2]));
        Jedilnik[2].concat(Line[i].substring(colEndIdx[2], colEndIdx[3]));
        Jedilnik[3].concat(Line[i].substring(colEndIdx[3], colEndIdx[4]));
        Jedilnik[4].concat(Line[i].substring(colEndIdx[4])); // to the end
      }
      // free mem
      for (i = 0; i < 25; i++) {
        Line[i].clear();
      }

      Serial.println("===== DEVEL 5 - text in columns with spaces ==================");
      for (i = 0; i < 5; i++) {
        Serial.println(Jedilnik[i]);
        Serial.println("---------------");
      }
      Serial.println("==============================================================");

// NEW ALGORYTHM END

      // clean un-wanted words and chars
      for (int i = 0; i < 5; i++)
      {
        int idx = Jedilnik[i].indexOf("sssz:");
        if (idx > 0) Jedilnik[i].remove(idx, 5);
        idx = Jedilnik[i].indexOf("*");
        if (idx > 0) Jedilnik[i].remove(idx, 1);
        idx = Jedilnik[i].indexOf("1");
        if (idx > 0) Jedilnik[i].remove(idx, 1);
      }

      for (int i = 0; i < 5; i++)
      {
        TrimNonPrintable(Jedilnik[i]);
        Jedilnik[i].trim(); // remove leading and trailing spaces
        TrimDoubleSpaces(Jedilnik[i]);
        // if no data is found, add a sad face, so that file load works normally
        if (Jedilnik[i].length() == 0) {Jedilnik[i] = "(O_o)";}
      }
      // clean un-wanted words #2
      int p11, p22;
      for (int i = 0; i < 5; i++)
      {
        p11 = Jedilnik[i].indexOf("dodatno"); // Dodatno iz EU Solske sheme: / Dodatno iz EU sheme:
        if (p11 >=0) {
          p22 = Jedilnik[i].indexOf(':', p11);
          if (p22 > p11) {
            Jedilnik[i].remove(p11, p22-p11+2);
          }
        }
      }
      // list extracted data
      Serial.println("-------------------");
      Serial.println(PDF_URL);
      Serial.println("-------------------");
      Serial.println(JedilnikDatum);
      Serial.println("-------------------");
      for (int i = 0; i < 5; i++)
      {
        Serial.println(Jedilnik[i]);
      }
      Serial.println("-------------------");

      Serial.println("Saving data into local file...");
      DisplayText("Saving data into local file...");
      fs::File file = SPIFFS.open(FileName, FILE_WRITE);
      if(!file){
          Serial.println("- failed to open file for writing");
          DisplayText("FAIL\n", CLRED);
          delay(2000);
          return;
      }
      bool ok = true;
      if(file.println(PDF_URL)){
          Serial.println("- file written");
          DisplayText(".");
      } else {
          Serial.println("- write failed");
          ok = false;         
      }
      if(file.println(JedilnikDatum)){
          Serial.println("- file written");
          DisplayText(".");
      } else {
          Serial.println("- write failed");
          ok = false;         
      }

      for (int i = 0; i < 5; i++)
      {
        if(file.println(Jedilnik[i])){
            Serial.println("- file written");
            DisplayText(".");
        } else {
            Serial.println("- write failed");
            ok = false;         
        }
      }
      file.close();
      if(ok) {
          Serial.println("File write OK");
          DisplayText("OK\n", CLGREEN);
      } else {
          Serial.println("File write FAIL");
          DisplayText("FAIL\n", CLRED);
          delay(2000);
          return;
      }

    } else { // ConvertPdfToTxt OK
      return;
    }
  } // (NeedFreshData && PdfOk)
}

#endif // DEVEL_JEDILNIK_OS


void DrawJedilnikOsDomzale(void) {
  Serial.println("DrawJedilnikOsDomzale()");
  DisplayClear();
  String sToday = ArsoWeather[0].DayName;
  sToday.toUpperCase();
  sToday.remove(3);
  Serial.print("Today = ");
  Serial.println(sToday);
  
  int idx1, idx2;
  String Jed[4];
  int processSingleDay = -1;
  // Monday .. Friday
  for (int day = 0; day < 5; day++) {
    if (sToday.indexOf(DAYS3[day]) == 0) {
      Serial.println("Workday = true");
      // show next day
        if ((CurrentHour > 16) && (day < 4)) {
          day++;
          sToday = DAYS3[day];
          Serial.println("day++");
        }
      processSingleDay = day;
      break; // day matched
    } // day matched
  } // for

  // Sunday
  if (sToday.indexOf(DAYS3[6]) == 0) {
    Serial.print("Today is Sunday");
    // show next day
      if (CurrentHour > 16) {
        processSingleDay = 0; // Monday
        sToday = DAYS3[0]; // Monday
        Serial.print(" -> show Monday");
      }
      Serial.println();
  }

  // process data for that day
  if (processSingleDay > -1) {
    Serial.println("----------------------");
    // split to separate meals
    idx1 = 0;
    idx2 = 0;
    for (int jed = 0; jed < 4; jed++)  // ZAJTRK, DOP. MALICA, KOSILO, POP. MALICA
    {
      Jed[jed].clear();
      idx2 = FindUppercaseChar(Jedilnik[processSingleDay], idx1+1);
      Serial.print("idx2 = ");
      Serial.println(idx2);
      if (idx2 < idx1) idx2 = Jedilnik[processSingleDay].length();
      Jed[jed] = Jedilnik[processSingleDay].substring(idx1, idx2);
      idx1 = idx2;
      Serial.println(Jed[jed]);
      Serial.println("----------------------");
    }
    // backup, if splitting does not work correctly
    if ((Jed[1].length() < 5) || (Jed[2].length() < 5)) {
      Jed[1] = Jedilnik[processSingleDay];
      Jed[2].clear();
    }
  }

  // display
  if (processSingleDay > -1) {
    DisplayText(JedilnikDatum.c_str(), 1, 110, 15, CLBLUE);
    DisplayText(sToday.c_str(), 1, 20, 15, CLGREY);
    DisplayText(Jed[1].c_str(), 1, 1,  50, CLYELLOW, true);
    DisplayText(Jed[2].c_str(), 1, 1, 130, CLCYAN, true);
  } else { // weekend
    DisplayText(JedilnikDatum.c_str(), 1, 10, 1, CLBLUE);
    DisplayText("\n\n\n======================================\n", CLGREY);
    for (int i = 0; i < 5; i++) {
      DisplayText(Jedilnik[i].c_str(), CLYELLOW);
      DisplayText("\n======================================\n", CLGREY);
    }
    delay(1500);
  }
}

void InvalidateJedilnikOS (void) {
  LastTimeJedilnikRefreshed = 0; // data is not valid
  ReloadRequired = true;
}