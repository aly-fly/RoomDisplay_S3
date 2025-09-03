

// https://developers.coinranking.com/api/documentation/coins/coin-price-history
// https://account.coinranking.com/dashboard/api
// Free (Current Plan) = 5000 API calls/month & 5 calls/second
// real usage = each hour = 744 calls / month


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

#define MAX_DATA_POINTS (31*24)  // real data = 720 points (each point is 1 hour)
#define REQ_DATA_POINTS   DspW  // enough to fill the display

float_t CoinData[MAX_DATA_POINTS];  // data for 1 month = 3 kB (4B / point)
unsigned int CoinDataLength = 0;

unsigned long LastTimeCoinRefreshed = 0; // data is not valid

// reference: "C:\Users\yyyyy\.platformio\packages\framework-arduinoespressif32\libraries\HTTPClient\examples\BasicHttpsClient\BasicHttpsClient.ino"
//            "C:\Users\yyyyy\.platformio\packages\framework-arduinoespressif32\libraries\HTTPClient\examples\StreamHttpClient\StreamHttpClient.ino"



#define COINRANKING_URL  "https://api.coinranking.com/v2/coin/Qwsogvtv82FCd/price-history?timePeriod=30d"

bool GetDataFromCoinServer(void) {
  bool result = false;
  unsigned long StartTime = millis();
  bool Timeout = false;
  bool Finished = false;
  String sBufff;

  CoinDataLength = 0;
    
  if (!WiFi.isConnected()) {
      return false;
  }

  setClock();
  if (!loadFileFromSDcardToMerory("/cert/api-coinranking-com.crt", Certificate, sizeof(Certificate), true))
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
      if (https.begin(*client, COINRANKING_URL)) {  // HTTPS
        // set hearders
//      https.addHeader("accept", "application/json");
        https.addHeader("x-access-token", COINRANKING_API_KEY_1);
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
                DisplayText("API credits left: ", CLYELLOW);
                DisplayText(https.header("X-RateLimit-Remaining-Month").c_str(), CLYELLOW);
                DisplayText("\n");
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
                  Serial.print("+");
                  DisplayText(".");
                  // process received data
                  if (sBufff.length() > 10) {
                    pos = 0;
                    sVal = FindJsonParam(sBufff, "price", pos);
                    //Serial.println(pos);
                    if (pos >= 0) {
                      TrimNumDot(sVal);  // delete everything except numbers and "."
                      #ifdef DEBUG_OUTPUT_DATA
                        if(CoinDataLength < 5) { 
                          Serial.println(sVal); 
                        }
                      #endif
                      Val = sVal.toFloat();
                      if ((Val > 10000) && (Val < 123000)) {
                        CoinData[CoinDataLength] = Val;
                        CoinDataLength++;
                        if (CoinDataLength > MAX_DATA_POINTS) {
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
                  } // if data available
                  // last section was received; it finishes off with "}]}}"
                  if (sBufff.indexOf("]") >= 0) {
                    Serial.println();
                    Serial.println("End identifier found.");
                    DisplayText("\nEnd identifier found.\n", CLGREEN);
                    Finished = true;
                  }  // end found
                  // timeout
                  Timeout = (millis() > (StartTime + 20 * 1000));  // 20 seconds
                  if (Timeout || Finished) { break; }
                } // while: connected or not finished
                Serial.println();
                Serial.printf("Number data points received: %d\n", CoinDataLength);
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
    CoinDataLength = 0;
  }
  sBufff.clear();
  Serial.print("Time needed (ms): ");
  Serial.println(millis() - StartTime);
  return result;
}






bool GetCoinData(void) {
    Serial.println("GetCoinData()");
    bool result = false;

    if (! HasTimeElapsed(&LastTimeCoinRefreshed, 1*60*60*1000)) {  // check server every hour
      Serial.println("Coin data is valid.");
      return true;  // data is already valid
    }

    Serial.println("Requesting data from CoinRanking server...");
    DisplayClear();
    DisplayText("Contacting CoinRanking server\n", CLYELLOW);
    if (!GetDataFromCoinServer()) {
        // LastTimeCoinRefreshed = 0; // retry
        DisplayText("FAILED!\n", CLRED);
        delay (2000);
        return false;
    }
    Serial.println("Number of data points: " + String(CoinDataLength));
    char Txt[20];
    sprintf(Txt, "Data points: %u\n", CoinDataLength);
    DisplayText(Txt);

    if (CoinDataLength > REQ_DATA_POINTS) {
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




void PlotCoinData(void) {
  Serial.println("PlotCoinData()");
  DisplayClear();

  if (CoinDataLength < DspW) {
    Serial.println("BTC: Not enough data to plot!");
    DisplayText("BTC: NOT ENOUGH DATA", FONT_TITLE, 5, 20, CLRED);
    delay(500);
    return;
  }

  char FileName[30];
  sprintf(FileName, "/bg_btc_%dx%d_w.bmp", DspW, DspH);
  DisplayShowImage(FileName,   0, 0);

  // vertical line
  // 1 px = 1 h;  168 px = 1 week
  int LineX1 = DspW - 168;
  int LineX2 = DspW - 168 * 2;
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("week", LineX1 + 4, DspH - 22, 1);
  int numDots = DspH / 6;
  for (int i = 0; i < numDots; i++)
  {
    tft.drawFastVLine(LineX1, i*6, 3, TFT_DARKGREY);
    tft.drawFastVLine(LineX2, i*6, 3, TFT_DARKGREY);
  }  

  float_t Minn, Maxx;
  Minn =  999999999;
  Maxx = -999999999;
  uint16_t IgnoreOldDataPoints = CoinDataLength - DspW;
  Serial.print("All data: ");
  Serial.println(CoinDataLength);
  Serial.print("Ignored old points: ");
  Serial.println(IgnoreOldDataPoints);

  // newest data is in the beginning; oldedt is at the end of the array
  for (uint16_t i = 0; i < (CoinDataLength - IgnoreOldDataPoints); i++)
  {
    if (CoinData[i] > Maxx) {Maxx = CoinData[i];}
    if (CoinData[i] < Minn) {Minn = CoinData[i];}
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
    fY = (CoinData[DspW - X - 1] - MinnD) * Scaling;
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
  X = tft.drawNumber(round(CoinData[0]), 250, 9);
  tft.drawString("USD", 250 + X + 13, 9);
  tft.unloadFont();
}




void InvalidateCoinData(void) {
  LastTimeCoinRefreshed = 0; // data is not valid
}
