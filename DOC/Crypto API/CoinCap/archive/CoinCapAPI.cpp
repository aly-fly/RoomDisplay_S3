

// https://pro.coincap.io/api-docs/
// https://pro.coincap.io/dashboard
// Free (Current Plan) = 2,500 credits/month*
// *Each API call charges a base cost of 1 credit, then 1 additional credit per 2500 bytes of data. Web-socket charges 1 credit per minute of use.
// 1 API call (1H) = 129 kB data = 52+1 = 53 credits = 47 API calls / month
// real data: 1 call = 41 credits

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

#define MAX_DATA_POINTS_1H (31*24)  // real data = 720 points
#define REQ_DATA_POINTS_1H   DspW  // enough to fill the display

float_t CoinCapData_1H[MAX_DATA_POINTS_1H];  // data for 1 month = 3 kB (4B / point)
unsigned int CoinCapDataLength_1H = 0;

unsigned long LastTimeCoinCapRefreshed_1H = 0; // data is not valid

// reference: "C:\Users\yyyyy\.platformio\packages\framework-arduinoespressif32\libraries\HTTPClient\examples\BasicHttpsClient\BasicHttpsClient.ino"
//            "C:\Users\yyyyy\.platformio\packages\framework-arduinoespressif32\libraries\HTTPClient\examples\StreamHttpClient\StreamHttpClient.ino"



#define COINCAP_1H_URL  "https://rest.coincap.io/v3/assets/bitcoin/history?interval=h1"

bool GetDataFromCoinCapServer(void) {
  bool result = false;
  unsigned long StartTime = millis();
  bool Timeout = false;
  bool Finished = false;
  String sBufff;

  CoinCapDataLength_1H = 0;
    
  if (!WiFi.isConnected()) {
      return false;
  }

  setClock();
  if (!loadFileFromSDcardToMerory("/cert/api-coincap-io.crt", Certificate, sizeof(Certificate), true))
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
      if (https.begin(*client, COINCAP_1H_URL)) {  // HTTPS
        // set hearders
        https.addHeader("accept", "application/json");
        https.addHeader("Authorization", COINCAP_API_KEY_1);
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
                Serial.println("[HTTPS] Streaming data from server.");
                DisplayText("Reading data", CLYELLOW);
                // get tcp stream
                WiFiClient * stream = https.getStreamPtr();
                // read all data from server
                bool firstBuffer = true;
                int pos;
                String sVal;
                float_t Val; 
                while ((!Finished) && https.connected()) {
                  sBufff = stream->readStringUntil('}');
                  #ifdef DEBUG_OUTPUT_DATA
                    // write it to Serial
                    if (firstBuffer) { Serial.println(sBufff); }
                    firstBuffer = false;
                  #endif
                  Serial.print(".");
                  DisplayText(".");
                  // process received data
                  if (sBufff.length() > 10) {
                    pos = 0;
                    sVal = FindJsonParam(sBufff, "priceUsd", pos);
                    //Serial.println(pos);
                    if (pos >= 0) {
                      TrimNumDot(sVal);  // delete everything except numbers and "."
                      #ifdef DEBUG_OUTPUT_DATA
                        if(CoinCapDataLength_1H < 5) { 
                          Serial.println(sVal); 
                        }
                      #endif
                      Val = sVal.toFloat();
                      if ((Val > 10000) && (Val < 123000)) {
                        CoinCapData_1H[CoinCapDataLength_1H] = Val;
                        CoinCapDataLength_1H++;
                        if (CoinCapDataLength_1H > MAX_DATA_POINTS_1H) {
                          Serial.println();
                          Serial.println("MAX NUMBER OF DATA POINTS REACHED!");
                          DisplayText("\nMax number of data points reached!\n");
                          Finished = true;
                          break;
                        } // max data points
                      } // value is sensible
                      else {
                        Serial.println();
                        Serial.print("Weird data: ");
                        Serial.print(sBufff);
                        Serial.print(" -> ");
                        Serial.print(sVal);
                        Serial.print(" -> ");
                        Serial.println(Val);
                      } // weird data
                    } // if data found
                    // last section was received; it finishes off with "timestamp"
                    if (sBufff.indexOf("timestamp") >= 0) {
                      Serial.println();
                      Serial.println("End identifier found.");
                      DisplayText("\nEnd identifier found.\n", CLGREEN);
                      Finished = true;
                    }  // end found
                  } // if data available
                  // timeout
                  Timeout = (millis() > (StartTime + 20 * 1000));  // 20 seconds
                  if (Timeout || Finished) { break; }
                } // connected or not finished
                Serial.println();
                if (Timeout) {
                  Serial.println("[HTTPS] Timeout.");
                  DisplayText("Timeout.\n");
                } else {
                  Serial.println("[HTTPS] Connection closed or file end.");
                }
            // streaming end
            DisplayText("\n");
            result = Finished;
          } // HTTP code > 0
          else {
            Serial.printf("Result / extra data provided by the server: %s \r\n", https.getString());
          }
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

  if (!result) {
    CoinCapDataLength_1H = 0;
  }
  sBufff.clear();
  Serial.print("Time needed (ms): ");
  Serial.println(millis() - StartTime);
  return result;
}






bool GetCoinCapData_1H(void) {
    Serial.println("GetCoinCapData_1H()");
    bool result = false;

    if (! HasTimeElapsed(&LastTimeCoinCapRefreshed_1H, 24*60*60*1000)) {  // check server every 1/2 hour
      Serial.println("CoinCap data 1H is valid.");
      return true;  // data is already valid
    }

    Serial.println("Requesting data 1H from CoinCap server...");
    DisplayClear();
    DisplayText("Contacting COINCAP server (1H)\n", CLYELLOW);
    if (!GetDataFromCoinCapServer()) {
        // LastTimeCoinCapRefreshed_1H = 0; // retry
        DisplayText("FAILED!\n", CLRED);
        delay (2000);
        return false;
    }
    Serial.println("Number of data points: " + String(CoinCapDataLength_1H));
    char Txt[20];
    sprintf(Txt, "Data points: %u\n", CoinCapDataLength_1H);
    DisplayText(Txt);

    if (CoinCapDataLength_1H > REQ_DATA_POINTS_1H) {
      result = true;
    } else {
      Serial.println("Not enough data points!");
      DisplayText("Not enough data points!\n", CLRED);
      delay (500);
      return false;
    }

    DisplayText("Finished\n", CLGREEN);
    delay (500);
    return result;
}




void PlotCoinCapData(const float *DataArray, const int DataLen, const int LineSpacing, const char BgImage) {
  Serial.println("PlotCoinCapData()");
  DisplayClear();

  if (DataLen < DspW) {
    Serial.println("BTC: Not enough data to plot!");
    DisplayText("BTC: NOT ENOUGH DATA", FONT_TITLE, 5, 20, CLRED);
    delay(500);
    return;
  }

  char FileName[30];
  sprintf(FileName, "/bg_btc_%dx%d_%c.bmp", DspW, DspH, BgImage);
  DisplayShowImage(FileName,   0, 0);

  // vertical line
  String sTxt;
  if (BgImage == 'd') {
    sTxt = "day";
  } else {
    sTxt = "week";
  }
  int LineX1 = DspW - LineSpacing;
  int LineX2 = DspW - LineSpacing * 2;
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString(sTxt, LineX1 + 4, DspH - 22, 1);
  int numDots = DspH / 6;
  for (int i = 0; i < numDots; i++)
  {
    tft.drawFastVLine(LineX1, i*6, 3, TFT_DARKGREY);
    tft.drawFastVLine(LineX2, i*6, 3, TFT_DARKGREY);
  }  

  float_t Minn, Maxx;
  Minn =  999999999;
  Maxx = -999999999;
  uint16_t IgnoreFirst = DataLen - DspW;
  Serial.print("All data: ");
  Serial.println(DataLen);
  Serial.print("Ignored first points: ");
  Serial.println(IgnoreFirst);

  for (uint16_t i = IgnoreFirst; i < DataLen; i++)
  {
    if (DataArray[i] > Maxx) {Maxx = DataArray[i];}
    if (DataArray[i] < Minn) {Minn = DataArray[i];}
  }
  Serial.println("Min: " + String(Minn));
  Serial.println("Max: " + String(Maxx));

  // for display only
  float MinnD = Minn - 200;
  float MaxxD = Maxx + 200;
  
  uint32_t color;
  int diff;
  uint16_t X, iY, oldY, Y, h;
  float_t Scaling, fY;
  Scaling = (float(DspH) / (MaxxD - MinnD));
  Serial.printf("Scaling: %f\r\n", Scaling);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  for (X = 0; X < DspW; X++) {
    fY = (DataArray[IgnoreFirst + X] - MinnD) * Scaling;
    iY = round(fY);
    if (iY < 0) iY = 0;
    if (iY >= DspH) iY = DspH-1;
    if (X == 0) {oldY = iY-1;}
    diff = iY - oldY;
    h = abs(diff);
    if (h == 0) {h = 1;}
     // Y0 on display is on the top -> mirror Y
    if (diff < 0) {
      color = TFT_RED; 
      Y = DspH - oldY;
    } else {
      color = TFT_GREEN; 
      Y = DspH - iY;
    }
    tft.drawFastVLine(X, Y, h, color);
    oldY = iY;
  }
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawNumber(round(Maxx), 75, 2, 1);
  tft.drawNumber(round(Minn), 75, DspH - 12, 1);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.loadFont(FN_TITLE);
  X = tft.drawNumber(round(DataArray[DataLen-1]), 250, 9);
  tft.drawString("USD", 250 + X + 13, 9);
  tft.unloadFont();
}



void PlotCoinCapData_1H(void) {
  PlotCoinCapData(CoinCapData_1H, CoinCapDataLength_1H, 168, 'w'); // 1 px = 1 h. 168 px = 1 week.
}


void InvalidateCoinCapData(void) {
  LastTimeCoinCapRefreshed_1H = 0; // data is not valid
}
