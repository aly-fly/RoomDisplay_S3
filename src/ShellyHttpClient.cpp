#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <utils.h>
#include "__CONFIG.h"
#include "myWiFi.h"

String ShellyJsonResponse;
String sTotalPower, sShellyTemperature;
float ShellyTotalPower; //, ShellyTemperature;
bool Shelly1ON = false, Shelly2ON = false;
float Shelly2Power = 0;

bool ShellyReadServer(String URL) {
    bool result = false;
    if (!WiFi.isConnected()) {
        return false;
    }
        HTTPClient http;
        Serial.println("Shelly connect...");
        http.setTimeout(1000); // ms?      (default HTTPCLIENT_DEFAULT_TCP_TIMEOUT = 5000)
        http.setConnectTimeout(1000); // ms (default HTTPCLIENT_DEFAULT_TCP_TIMEOUT = 5000)
        if (http.begin(URL)) {
            Serial.println("[HTTP] GET...");
            // start connection and send HTTP header
            int httpCode = http.GET();
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK) {
                ShellyJsonResponse = http.getString();
                result = (ShellyJsonResponse.length() > 10);
            } else {
                Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }
        } else {
        Serial.println("Connect failed!");
        }
        http.end();
        return result;
    }


bool ShellyGetPower(void) {
    Serial.println("ShellyGetPower()");
    bool result = false;
    sTotalPower = "n/a";
    ShellyTotalPower = 0;
    if (ShellyReadServer(SHELLY_3EM_URL)) {
        // Serial.println(JsonData);
        // do nout use JSON parser, as we are only interested in one segment ("total_power":74.62,)
        int idx = ShellyJsonResponse.indexOf("total_power");
        if (idx > 0) {
            unsigned int idxBegin = ((unsigned int) idx) + 13;
            unsigned int idxEnd = ShellyJsonResponse.indexOf(",", idxBegin);
            sTotalPower = ShellyJsonResponse.substring(idxBegin, idxEnd);
            TrimNumDot(sTotalPower);
            ShellyTotalPower = sTotalPower.toFloat();
            Serial.print("Data: ");
            Serial.print(sTotalPower);
            Serial.print(" = ");
            Serial.print(ShellyTotalPower);
            Serial.println();
            result = true;
        }
    }
    ShellyJsonResponse.clear();  // free memory
    return result;
}

bool ShellyGetTemperature(void) {
    Serial.println("ShellyGetTemperature()");
    bool result = false;
    sShellyTemperature = "----";
    //ShellyTemperature = 0;
    if (ShellyReadServer(SHELLY_1PM_ADDON_URL)) {
        // Serial.println(JsonData);
        // {"id": 101,"tC":25.2, "tF":77.3}
        // do nout use JSON parser, as we are only interested in one segment ("tC":25.2,)
        int idx = ShellyJsonResponse.indexOf("tC");
        if (idx > 0) {
            unsigned int idxBegin = ((unsigned int) idx) + 4;
            unsigned int idxEnd = ShellyJsonResponse.indexOf(",", idxBegin);
            sShellyTemperature = ShellyJsonResponse.substring(idxBegin, idxEnd);
            TrimNumDot(sShellyTemperature);
            // ShellyTemperature = sShellyTemperature.toFloat();
            sShellyTemperature.concat (" C");
            Serial.print("Data: ");
            Serial.print(sShellyTemperature);
            /*
            Serial.print(" = ");
            Serial.print(ShellyTemperature);
            */
            Serial.println();
            result = true;
        }
    }
    ShellyJsonResponse.clear();  // free memory
    return result;
}

bool ShellyGetSwitch1(void) {
    Serial.println("ShellyGetSwitch1()");
    bool result = false;
    if (ShellyReadServer(SHELLY_1PM_SW1_URL)) {
        // {"id":0, "source":"SHC", "output":true, "timer_started_at":1716815065.93, "timer_duration":10800.00, "apower":177.8, "voltage":225.9, "current":0.797, .........
        int idx = ShellyJsonResponse.indexOf("output");
        if (idx > 0) {
            ShellyJsonResponse.remove(0, idx + 8);
            idx = ShellyJsonResponse.indexOf(",");
            ShellyJsonResponse.remove(idx);
            Serial.print("Output 1: ");
            Serial.println(ShellyJsonResponse);
            Shelly1ON = (ShellyJsonResponse == "true");
            result = true;
        }
    }
    ShellyJsonResponse.clear();  // free memory
    return result;
}

bool ShellyGetSwitch2(void) {
    Serial.println("ShellyGetSwitch2()");
    bool result = false;
    int idx;
    unsigned int idxBegin, idxEnd;
    String tempStr;
    if (ShellyReadServer(SHELLY_1PM_SW2_URL)) {
        // {"id":0, "source":"loopback", "output":false, "apower":0.0, "voltage":228.5, "current":0.000, "aenergy":{"total":68729.347,"by_minute":[0.000,0.000,0.000],"minute_ts":1718004720},"temperature":{"tC":50.3, "tF":122.5}}
        idx = ShellyJsonResponse.indexOf("output");
        if (idx > 0) {
            idxBegin = ((unsigned int) idx) + 8;
            idxEnd = ShellyJsonResponse.indexOf(",", idxBegin);
            tempStr = ShellyJsonResponse.substring(idxBegin, idxEnd);
            Serial.print("Output 2: ");
            Serial.println(tempStr);
            Shelly2ON = (tempStr == "true");
            result = true;
        }
        idx = ShellyJsonResponse.indexOf("apower");
        if (idx > 0) {
            idxBegin = ((unsigned int) idx) + 8;
            idxEnd = ShellyJsonResponse.indexOf(",", idxBegin);
            tempStr = ShellyJsonResponse.substring(idxBegin, idxEnd);
            TrimNumDot(tempStr);
            Shelly2Power = tempStr.toFloat();
            Serial.print("Power 2: ");
            Serial.print(tempStr);
            Serial.print(" = ");
            Serial.print(Shelly2Power);
            Serial.println();
            result = true;
        }
    }
    ShellyJsonResponse.clear();  // free memory
    return result;
}
