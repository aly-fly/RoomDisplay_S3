#include <Arduino.h>
#include <stdint.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "__CONFIG.h"
#include "myWiFi.h"
#include <utils.h>
#include "Clock.h"
#include "SD_Card.h"
#include "GlobalVariables.h"
#include "display.h"


#define   TestHost1 "connectivitycheck.gstatic.com"
IPAddress TestHost1IP(142,251,208,131);

#define   TestHost2 "clients3.google.com"
IPAddress TestHost2IP(142,251,39,78);

int httpCode;
String ResponseFromServer;
String CaptivePortalURL;

bool StatusDNSvalid = false;
bool StatusConnectivityOk = false;

// ***********************************************************************************************************************************************************

bool CheckForValidDNSresponses(void) {
    Serial.println("Checking for captive portal..");
    DisplayText("Checking for captive portal..\n");
    if (!WiFi.isConnected()) {
        Serial.println("NO WiFi!");
        DisplayText("NO WiFi!\n", CLRED);
        delay(2000);
        return false;
    }

    IPAddress ResolvedIP((uint32_t)0);

    Serial.print("Resolving test host 1: ");
    Serial.println(TestHost1);
    if (WiFi.hostByName(TestHost1, ResolvedIP)) {
        Serial.print("Resolved to: ");
        Serial.println(ResolvedIP.toString());
        if (ResolvedIP[0] == TestHost1IP[0]) {  // verify first byte only
            Serial.println("OK1");
            DisplayText("OK1\n", CLGREEN);
            return true;
        } else {
        Serial.println("IP does not match");
        }
    } else {
    Serial.println("Resolving failed");
    }


    Serial.print("Resolving test host 2: ");
    Serial.println(TestHost2);
    if (WiFi.hostByName(TestHost2, ResolvedIP)) {
        Serial.print("Resolved to: ");
        Serial.println(ResolvedIP.toString());
        if (ResolvedIP[0] == TestHost2IP[0]) {  // verify first byte only
            Serial.println("OK2");
            DisplayText("OK2\n", CLGREEN);
            return true;
        } else {
        Serial.println("IP does not match");
        }
    } else {
    Serial.println("Resolving failed");
    }

    Serial.println("DNS test failed!");
    DisplayText("DNS test failed!\n", CLRED);
    delay(500);
    return false;
}

// ***********************************************************************************************************************************************************

bool HTTPSconnect(String URL) {
  if (!WiFi.isConnected()) {
      return false;
  }
  bool result = false;
  loadFileFromSDcardToMerory("/cert/rls.crt", Certificate, sizeof(Certificate), true);

  Serial.println("Connecting to: " + URL);

  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client->setHandshakeTimeout(10); // seconds (default 120 s)
    client->setTimeout(10);          // seconds (default  30 s)
    client->setCACert(Certificate);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
  
      Serial.print("[HTTPS] begin...\r\n");
      if (https.begin(*client, URL)) {  // HTTPS
        delay(1); // watchdog reset
        Serial.print("[HTTPS] GET...\r\n");
        // start connection and send HTTP header
        httpCode = https.GET();
        delay(1); // watchdog reset

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
          if ((httpCode == HTTP_CODE_OK) || (httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            ResponseFromServer = https.getString();
            result = ResponseFromServer.length() > 100;
            
            Serial.println("--- data begin ---");
            Serial.println(ResponseFromServer);
            Serial.println("--- data data end ---");
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\r\n", https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\r\n");
      }
      // End extra scoping block
    }
  
    delete client;
  } else {
    Serial.println("Unable to create HTTPS client");
  }
    return result;
}

// ***********************************************************************************************************************************************************

int HTTPconnect(String URL) {
  int result = -1;
  if (!WiFi.isConnected()) {
    return result;
  }
  Serial.println("Connecting to: " + URL);
  HTTPClient http;
  if (http.begin(URL)) {
    Serial.println("[HTTP] GET...");
    // start connection and send HTTP header
    int httpCode = http.GET();
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if ((httpCode == HTTP_CODE_MOVED_PERMANENTLY) ||
        (httpCode == HTTP_CODE_FOUND) ||                 // 'HTTP/1.1 302 Captive Portal'
        (httpCode == HTTP_CODE_TEMPORARY_REDIRECT) ||
        (httpCode == HTTP_CODE_PERMANENT_REDIRECT)) {              
      CaptivePortalURL = http.getLocation(); // get the data in the "Location: xxx" header
      Serial.print("Found Redirect: ");
      Serial.println(CaptivePortalURL);             
      result = 22; // found redirect notice
    } else {
      if (httpCode == HTTP_CODE_OK) {
        ResponseFromServer = http.getString();
        Serial.println("--- data begin ---");
        Serial.println(ResponseFromServer);
        Serial.println("--- data data end ---");
        if (ResponseFromServer.length() == 0) result = -4; else result = 11; // Connectivity ok            
      } else {
        Serial.printf("[HTTP] Unknown code returned: %s\n", http.errorToString(httpCode).c_str());
        result = -3;
      }
    }
  } else {
  Serial.println("Connect failed!");
  result = -2;
  }
  http.end();
  return result;
}

// ***********************************************************************************************************************************************************

int HTTPconnectPOST(String URL, String PostData) {
  int result = -1;
  if (!WiFi.isConnected()) {
    return result;
  }
  Serial.println("Connecting to: " + URL);
  Serial.println("Sending POST: " + PostData);
  HTTPClient http;
  if (http.begin(URL)) {
    Serial.println("[HTTP] POST...");
    // start connection and send HTTP header
    int httpCode = http.POST(PostData);
    if (httpCode == HTTP_CODE_OK) {
      ResponseFromServer = http.getString();
      Serial.println("--- data begin ---");
      Serial.println(ResponseFromServer);
      Serial.println("--- data data end ---");
      if (ResponseFromServer.length() == 0) result = -4; else result = 11; // Connectivity ok            
    }
    result = 11;
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  } else {
  Serial.println("Connect failed!");
  result = -2;
  }
  http.end();
  return result;
}

// ***********************************************************************************************************************************************************


bool CheckConnectivityAndHandleCaptivePortalLogin(void) {
  Serial.println("Captive portal Begin");
  DisplayText("Captive portal Begin\n");
  if (!WiFi.isConnected()) {
      Serial.println("No WiFi!");
      DisplayText("NO WiFi!\n", CLRED);
      StatusConnectivityOk = false;
      return false;
  }

  StatusDNSvalid = CheckForValidDNSresponses();

  int ResCode = -99;
  Serial.println("Creating dummy HTTP request to get Redirect information...");
  DisplayText("Checking for Redirect notice...\n");
  String URL1 = "http://";
  URL1.concat(TestHost2);  // host 1 produces 404, host 2 returns empty page
  ResCode = HTTPconnect(URL1);
  Serial.print("Res code: ");
  Serial.println(ResCode);
  DisplayText("Result code: ");
  DisplayText(String(ResCode).c_str());
  DisplayText("\n");

  /* response - headers
  Connecting to: http://connectivitycheck.gstatic.com
  [  2894][V][HTTPClient.cpp:252] beginInternal(): url: http://connectivitycheck.gstatic.com
  [  2895][D][HTTPClient.cpp:303] beginInternal(): protocol: http, host: connectivitycheck.gstatic.com port: 80 url: /
  [HTTP] GET...
  [  2913][D][HTTPClient.cpp:598] sendRequest(): request type: 'GET' redirCount: 0

  [  2923][D][HTTPClient.cpp:1170] connect():  connected to connectivitycheck.gstatic.com:80
  [  2938][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'HTTP/1.1 302 Captive Portal'
  [  2939][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'Server:'
  [  2942][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'Date: Tue, 12 Mar 2024 08:36:57 GMT'
  [  2950][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'Cache-Control: no-cache,no-store,must-revalidate,post-check=0,pre-check=0'
  [  2963][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'Location: https://wifi.rls.si/swarm.cgi?opcode=cp_generate&orig_url=687474703a2f2f636f6e6e6563746976697479636865636b2e677374617469632e636f6d2f'
  [  2980][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'Content-Type: text/html; charset=utf-8'
  [  2989][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'X-Frame-Options: SAMEORIGIN'
  [  2997][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'X-XSS-Protection: 1; mode=block'
  [  3006][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'X-Content-Type-Options: nosniff'
  [  3014][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'Strict-Transport-Security: max-age=31536000'
  [  3023][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: 'Connection: close'
  [  3030][V][HTTPClient.cpp:1264] handleHeaderResponse(): RX: ''
  [  3036][D][HTTPClient.cpp:1321] handleHeaderResponse(): code: 302
  */

  if ((StatusDNSvalid) && (ResCode == 11)) { // connectivity ok - no need to do anything
    StatusConnectivityOk = true;
    Serial.println("Connectivity OK.");
    DisplayText("Connectrivity OK\n", CLGREEN);
    return true;
  }
  if (ResCode == 22) { // Got redirect URL
    Serial.println("Opening Captive portal..");
    DisplayText("Opening Captive portal...\n", CLYELLOW);
    CaptivePortalURL.replace("https", "http");
    ResCode = HTTPconnect(CaptivePortalURL);
    Serial.print("Res code: ");
    Serial.println(ResCode);
    DisplayText("Result code: ");
    DisplayText(String(ResCode).c_str());
    DisplayText("\n");

    if (ResCode == 11) { // Captive portal access OK
      Serial.println("Sending required POST message...");
      DisplayText("Sending Login message...\n", CLYELLOW);
      /*
      <form method="POST" action="swarm.cgi"> 
        <input type="hidden" name="orig_url" value="687474703a2f2f636c69656e7473332e676f6f676c652e636f6d2f" /> 
        <input type="hidden" name="opcode" value="cp_ack" /> 
        <input type="submit" value="Accept"/> 
      </form> 
      */
      String POSTkey, POSTurl, POSTdata;
      int PosPost, PosKey, PosKeyValueBegin, PosKeyValueEnd;

      PosPost          = ResponseFromServer.indexOf("form method=");
      //PosPost          = ResponseFromServer.indexOf("method=""POST""");
      PosKey           = ResponseFromServer.indexOf("orig_url", PosPost);
      PosKeyValueBegin = ResponseFromServer.indexOf("value=", PosKey);
      PosKeyValueEnd   = ResponseFromServer.indexOf("/>", PosKeyValueBegin);

      Serial.printf("Data index: %d, %d, %d, %d", PosPost, PosKey, PosKeyValueBegin, PosKeyValueEnd);
      Serial.println();

      POSTkey = ResponseFromServer.substring(PosKeyValueBegin + 7, PosKeyValueEnd);
      TrimAlfaNum (POSTkey);
      Serial.print("Key = ");
      Serial.println(POSTkey);
      ResponseFromServer.clear(); // free mem

      if ((PosKeyValueEnd < 40) || (POSTkey.length() < 50) || (POSTkey.length() > 60)){
        Serial.println("Error in data!");
        DisplayText("ERROR IN DATA\n", CLRED);
        StatusConnectivityOk = false;
        return false;
      }

      POSTurl = "http://wifi.rls.si/swarm.cgi";
      POSTdata = "orig_url=" + POSTkey + "&opcode=cp_ack";  // "orig_url=xxxxxxxx&opcode=cp_ack"
      Serial.print("POST data = ");
      Serial.println(POSTdata);


      ResCode = HTTPconnectPOST(POSTurl, POSTdata);
      Serial.print("Res code: ");
      Serial.println(ResCode);
      DisplayText("Result code: ");
      DisplayText(String(ResCode).c_str());
      DisplayText("\n");

      // .....  is there anything left to analyze in the data returned from POST method?

      Serial.println("Another dummy HTTP request to verify connectivity...");
      DisplayText("Checking connectivity...\n");
      String URL1 = "http://";
      URL1.concat(TestHost2);  // host 1 produces 404, host 2 returns empty page
      ResCode = HTTPconnect(URL1);
      Serial.print("Res code: ");
      Serial.println(ResCode);
      DisplayText("Result code: ");
      DisplayText(String(ResCode).c_str());
      DisplayText("\n");
      ResponseFromServer.clear(); // free mem

      if ((StatusDNSvalid) && (ResCode == 11)) { // connectivity ok
        StatusConnectivityOk = true;
        Serial.println("Connectivity OK.");
        DisplayText("Connectivity OK.\n", CLGREEN);
        return true;
      }
    }
  }
  DisplayText("ERROR\n", CLRED);
  return false;
}



