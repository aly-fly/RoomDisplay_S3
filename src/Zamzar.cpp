#include <Arduino.h>
#include <stdint.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "myWiFi.h"
#include <utils.h>
#include "Clock.h"
#include "display.h"
#include "SD_Card.h"
#include "GlobalVariables.h"
#include "___CONFIG_SECRETS.h"


  const String JobEndpointTest = "sandbox.zamzar.com/v1/jobs";
  const String FileEndpointTest = "sandbox.zamzar.com/v1/files";
  const String JobEndpoint = "api.zamzar.com/v1/jobs";
  const String FileEndpoint = "api.zamzar.com/v1/files";

  String JobID, FileID;


/*
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_client.html#http-authentication

Examples of Authentication Configuration
Authentication with URI

esp_http_client_config_t config = {
    .url = "http://user:passwd@httpbin.org/basic-auth/user/passwd",
    .auth_type = HTTP_AUTH_TYPE_BASIC,
};
Authentication with username and password entry

esp_http_client_config_t config = {
    .url = "http://httpbin.org/basic-auth/user/passwd",
    .username = "user",
    .password = "passwd",
    .auth_type = HTTP_AUTH_TYPE_BASIC,
};
*/

String ZamzarData;

bool HTTPSconnect(const String URL, const bool PostRequest, const String PostData, const bool PrintZamzarData) {
  Serial.println("Zamzar: HTTPSconnect()");
  bool result = false;
  
    if (!WiFi.isConnected()) {
        return false;
    }

  setClock();

  loadFileFromSDcardToMerory("/cert/api-zamzar-com.crt", Certificate, sizeof(Certificate));

  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client->setHandshakeTimeout(10000); // 10 seconds (default 120 s)
    client -> setCACert(Certificate);

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
      int httpCode;
  
      Serial.print("[HTTPS] begin...\r\n");
      if (https.begin(*client, URL)) {  // HTTPS
        // start connection and send HTTP header
        if (PostRequest) {
            Serial.print("[HTTPS] POST...\r\n");
            httpCode = https.POST(PostData);
        } else {
            Serial.print("[HTTPS] GET...\r\n");
            httpCode = https.GET();
        }  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\r\n", httpCode);
  
          // file found at server
          if ((httpCode == HTTP_CODE_OK) || (httpCode == HTTP_CODE_CREATED) || (httpCode == HTTP_CODE_ACCEPTED) || (httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            ZamzarData = https.getString();
            result = ZamzarData.length() > 50;
            if (PrintZamzarData) {
              Serial.println("--- data begin ---");
              Serial.println(ZamzarData);
              Serial.println("--- data end ---");
            }
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


/*
 https://developers.zamzar.com/docs#section-Start_a_conversion_job
 https://developers.zamzar.com/docs#section-Download_the_converted_file

1. start the job; get job ID
    curl https://sandbox.zamzar.com/v1/jobs -u eed6d139dc0b59597bce6d5c598b735da678d496: -X POST --data-urlencode "source_file=https://www.os-domzale.si/files/2024/04/jedilnik-2024-5-1.pdf" -d "target_format=txt"
    --> {"id":43539451,"key":"eed6d139dc0b59597bce6d5c598b735da678d496","status":"initialising","sandbox":true,"created_at":"2024-05-09T16:27:57Z","finished_at":null,
    "import":{"id":4655619,"url":"https:\/\/www.os-domzale.si\/files\/2024\/04\/jedilnik-2024-5-1.pdf","status":"initialising"},
    "target_files":[],"target_format":"txt","credit_cost":0}

2. poll the job ID status; get file ID
    curl https://sandbox.zamzar.com/v1/jobs/43533821 -u eed6d139dc0b59597bce6d5c598b735da678d496:
    --> {"id":43533821,"key":"eed6d139dc0b59597bce6d5c598b735da678d496","status":"successful","sandbox":false,"created_at":"2024-05-09T12:58:13Z","finished_at":"2024-05-09T12:58:20Z",
    "import":{"id":4655291,"url":"https:\/\/www.os-domzale.si\/files\/2024\/04\/jedilnik-2024-5-1.pdf","status":"successful"},
    "source_file":{"id":169760674,"name":"jedilnik-2024-5-1.pdf","size":225674},
    "target_files":[{"id":169760676,"name":"jedilnik-2024-5-1.txt","size":3500}],
    "target_format":"txt","credit_cost":1}

    --> {"id":43542499,"key":"eed6d139dc0b59597bce6d5c598b735da678d496","status":"failed",
    "failure":{"code":3,"message":"The source file could not be imported."},"sandbox":true,"created_at":"2024-05-09T18:31:49Z","finished_at":null,
    "import":{"id":4655866,"url":"https:\/\/www.os-domzale.si\/files\/2024\/04\/jedilnik-2024-5-.pdf","status":"failed",
    "failure":{"code":5,"message":"Could not find 'files\/2024\/04\/jedilnik-2024-5-.pdf' on the server at 'https:\/\/www.os-domzale.si'"}},
    "target_files":[],"target_format":"txt","credit_cost":0}

    --> {"errors":[{"code":21,"message":"resource does not exist"}]}

3. download the file ID
    curl https://sandbox.zamzar.com/v1/files/169760676/content -u eed6d139dc0b59597bce6d5c598b735da678d496: -L -i  
*/

bool ConvertPdfToTxt(const String PdfUrl) { 
    Serial.println("Zamzar: ConvertPdfToTxt()");
    bool ok;

    Serial.println("Sending file for online conversion");
    DisplayText("Sending file for online conversion\n");

    ZamzarData.clear();
    String URL = "https://" + ZamzarApiKey1 + ":@" + JobEndpoint;
    String POSTdata = "source_file=" + PdfUrl + "&target_format=txt";
    Serial.print("URL: ");
    Serial.println(URL);    
    Serial.print("POST data: ");
    Serial.println(POSTdata);    
    ok = HTTPSconnect(URL, true, POSTdata, true);
    if (!ok) {
      Serial.println("Fail Zamzar step 1: POST");
      DisplayText("Fail Zamzar step 1: POST\n", CLRED);
      DisplayText(ZamzarData.c_str(), CLYELLOW);
      ZamzarData.clear();  // free mem
      delay(8000);
      return false;    
    }
    POSTdata.clear(); // free mem
    ok = false;
    int idx1, idx2;
    idx1 = ZamzarData.indexOf("{\"id\":");
    idx2 = ZamzarData.indexOf(",");
    if ((idx1 >= 0) && (idx2 > idx1)) {
      JobID = ZamzarData.substring(idx1+6, idx2);
      Serial.print ("Job ID: ");
      Serial.println(JobID);
      DisplayText("\nJob ID:");
      DisplayText(JobID.c_str(), CLCYAN);
      DisplayText("\n");
      ok = true;
    }
    if (!ok) {
      Serial.println("Fail Zamzar step 1b: Finding Job ID");
      DisplayText("Fail Zamzar step 1b: Finding Job ID\n", CLRED);
      DisplayText(ZamzarData.c_str(), CLYELLOW);
      ZamzarData.clear();  // free mem
      delay(8000);
      return false;    
    }
    Serial.println("OK");
    DisplayText("OK\n", CLGREEN);
    // =========================================================
    URL = "https://" + ZamzarApiKey1 + ":@" + JobEndpoint + "/" + JobID;
    Serial.print("URL: ");
    Serial.println(URL);    
    bool Finished = false;
    Serial.println("Waiting for online conversion to be completed");
    DisplayText("Waiting for online conversion to be completed");

    int RetryCont = 0;
    int p = 0;
    while (!Finished) {
      DisplayText(".");
      RetryCont++;
      ZamzarData.clear();
      delay(2000);
      ok = HTTPSconnect(URL, false, "", true);
      if (!ok) {
        Serial.println("Fail Zamzar step 2: GET job status");
        DisplayText("\nFail Zamzar step 2: GET job status\n", CLRED);
        DisplayText(ZamzarData.c_str(), CLYELLOW);
        ZamzarData.clear();  // free mem
        delay(8000);
        return false;    
      }
   
      p = ZamzarData.indexOf("\"status\":\"successful\"");
      Finished = (p > 0) && (p < 130); // job finished, not import finished
      ok = ((ZamzarData.indexOf("\"status\":\"initialising\"") > 0) || Finished);

      if (!ok) {
        Serial.println("Fail Zamzar step 2b: job not successful");
        DisplayText("\nFail Zamzar step 2b: job not successful\n", CLRED);
        DisplayText(ZamzarData.c_str(), CLYELLOW);
        ZamzarData.clear();  // free mem
        delay(8000);
        return false;    
      }

      if (Finished) {
        ok = false;
        idx1 = ZamzarData.indexOf("\"target_files\":[{\"id\":");
        idx2 = ZamzarData.indexOf(",", idx1+1);
        Serial.println(idx1);
        Serial.println(idx2);
        if ((idx1 > 0) && (idx2 > (idx1+22))) {
          FileID = ZamzarData.substring(idx1+22, idx2);
          Serial.print ("File ID: ");
          Serial.println(FileID);
          DisplayText("\nFile ID:");
          DisplayText(FileID.c_str(), CLCYAN);
          DisplayText("\n");
          ok = true;
        }
        if (!ok) {
          Serial.println("Fail Zamzar step 2c: Finding File ID");
          DisplayText("\nFail Zamzar step 2c: Finding File ID\n", CLRED);
          ZamzarData.clear();  // free mem
          delay(8000);
          return false;    
        }
      }
      if (RetryCont > 7) {
        Serial.println("Fail Zamzar step 2d: Too many retries");
        DisplayText("\nFail Zamzar step 2d: Too many retries\n", CLRED);
        DisplayText(ZamzarData.c_str(), CLYELLOW);
        ZamzarData.clear();  // free mem
        delay(8000);
        return false;    
      }
    } // while in progress
    Serial.println("OK");
    DisplayText("OK\n", CLGREEN);
    // =========================================================
    Serial.println("Downloading the file");
    DisplayText("Downloading the file\n");
    ZamzarData.clear();  // free mem
    URL = "https://" + ZamzarApiKey1 + ":@" + FileEndpoint + "/" + FileID + "/" + "content";
    Serial.print("URL: ");
    Serial.println(URL);    
    ok = HTTPSconnect(URL, false, "", false);
    if (!ok) {
      Serial.println("Fail Zamzar step 3: GET file");
      DisplayText("Fail Zamzar step 3: GET file\n", CLRED);
      return false;    
    }
    Serial.println("OK");
    DisplayText("OK\n", CLGREEN);
    return true;
}

