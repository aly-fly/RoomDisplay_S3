#include <Arduino.h>
#include <stdint.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_sntp.h>
#include "esp_wps.h"
#include "myWiFi.h"
#include "display.h"

WiFiMulti wifiMulti;

//WifiState_t WifiState = disconnected;
//uint32_t TimeOfWifiReconnectAttempt = 0;
bool inHomeLAN = false;


void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info){
  IPAddress myIP;
  switch(event){
    case ARDUINO_EVENT_WIFI_STA_START:
      //WifiState = disconnected;
      Serial.println("Station Mode Started");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED: // IP not yet assigned
      Serial.println("Connected to AP: " + String(WiFi.SSID()));
      break;     
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      myIP = WiFi.localIP();
      Serial.print("Got IP: ");
      Serial.println(myIP);
      inHomeLAN = ((myIP[0] == HomeIP0) && (myIP[1] == HomeIP1));
      //WifiState = connected;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      //WifiState = disconnected;
      wifi_err_reason_t reasonNum = info.wifi_sta_disconnected.reason;
      const char* reasonTxt = WiFi.disconnectReasonName(reasonNum); // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html#wi-fi-reason-code

      Serial.print("WiFi lost connection. Reason: ");
      Serial.print(reasonNum);
      Serial.print(" - ");
      Serial.println(reasonTxt);
      DisplayClear();
      DisplayText("WiFi lost connection. Reason: ");
      DisplayText(String(reasonNum).c_str());
      DisplayText(" - ");
      DisplayText(reasonTxt);
      DisplayText("\n");
      break;
    default:
      break;
  }
}


// !! The bit 0 of the first byte of ESP32 MAC address can not be 1. For example, the MAC address can set to be “1a:XX:XX:XX:XX:XX”, but can not be “15:XX:XX:XX:XX:XX”.
uint8_t newMACAddress[] = {0x50, 0xCD, 0x98, 0x76, 0x54, 0x10};

void WifiInit(void)  {
/*
  // Get MAC address of the WiFi station interface
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
*/  

  WiFi.mode(WIFI_OFF);
  Serial.print("My old MAC = ");
  Serial.println(WiFi.macAddress());

//  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, newMACAddress);
  if (err == ESP_OK) {
      ESP_LOGI("MAC address", "MAC address successfully set.");
  } else {
      ESP_LOGE("MAC address", "Failed to set MAC address");
  }

  Serial.print("My new MAC = ");
  Serial.println(WiFi.macAddress());

  //WifiState = disconnected;
  DisplayText("WiFi start");

  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);  
  WiFi.setHostname(DEVICE_NAME);  
  WiFi.onEvent(WiFiEvent);

#ifdef WIFI_SSID2
  Serial.println("Multi WiFi start...");
  wifiMulti.addAP(WIFI_SSID1, WIFI_PASSWD1);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASSWD2);
#ifdef WIFI_SSID3
  wifiMulti.addAP(WIFI_SSID3, WIFI_PASSWD3);
#endif
  if(wifiMulti.run() != WL_CONNECTED) {
    Serial.println("\r\nWiFi connection timeout!");
    DisplayText("\nTIMEOUT!", CLRED);
    delay(2000);
    //WifiState = disconnected;
    return; // exit loop, exit procedure, continue startup
  }
#else
  Serial.print("WiFi start");
  WiFi.begin(WIFI_SSID1, WIFI_PASSWD1); 
  unsigned long StartTime = millis();
  while ((WiFi.status() != WL_CONNECTED)) {
    delay(500);
    DisplayText(".");
    Serial.print(".");
    if ((millis() - StartTime) > (WIFI_CONNECT_TIMEOUT_SEC * 1000)) {
      Serial.println("\r\nWiFi connection timeout!");
      DisplayText("\nTIMEOUT!", CLRED);
      delay(2000);
      //WifiState = disconnected;
      return; // exit loop, exit procedure, continue startup
    }
  }
#endif

  
  //WifiState = connected;

  DisplayText("\n Connected to: ");
  DisplayText(WiFi.SSID().c_str(), CLCYAN);
  DisplayText("\n IP: ");
  DisplayText(WiFi.localIP().toString().c_str(), CLCYAN);
  DisplayText("\n");
  
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
  delay(200);
}

void WifiReconnectIfNeeded(void) {
  if (!WiFi.isConnected()) {
    Serial.println("Attempting WiFi reconnection...");
    DisplayClear();
    DisplayText("WiFi reconnect...", CLYELLOW);
    WiFi.reconnect();
  }    
}


