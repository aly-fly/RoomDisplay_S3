#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <utils.h>
#include "__CONFIG.h"
#include "myWiFi.h"

String RadioResponse;

bool RadioGet(String URL) {
    bool result = false;
    RadioResponse.clear();
    if (!WiFi.isConnected()) {
        return false;
    }
        HTTPClient http;
        Serial.println("Radio connect...");
        http.setTimeout(1000); // ms?      (default HTTPCLIENT_DEFAULT_TCP_TIMEOUT = 5000)
        http.setConnectTimeout(1000); // ms (default HTTPCLIENT_DEFAULT_TCP_TIMEOUT = 5000)
        if (http.begin(URL)) {
            Serial.println("[HTTP] GET...");
            // start connection and send HTTP header
            int httpCode = http.GET();
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK) {
                RadioResponse = http.getString();
                result = (RadioResponse.length() > 0);
            } else {
                Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }
        } else {
        Serial.println("Connect failed!");
        }
        http.end();
        return result;
    }

