#include <Arduino.h>
#include <stdint.h>
#include <esp_sntp.h>
#include <time.h>
#include "__CONFIG.h"
#include "display.h"
#include "myWiFi.h"
#include "utils.h"

unsigned long LastTimeClockSynced = 0; // data is not valid


// Required for WiFiClientSecure and for checks the validity date of the certificate. 
// Setting clock for CA authorization...
void setClock(void) {
  Serial.println("setClock()");

  /*
  if (!inHomeLAN) {
    sntp_servermode_dhcp(1); //try to get the ntp server from dhcp
    sntp_setservername(1, TIME_SERVER); //fallback server
    sntp_init();

    // test sntp server
    // https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/NTP-TZ-DST/NTP-TZ-DST.ino

    // lwIP v2 is able to list more details about the currently configured SNTP servers
    for (int i = 0; i < SNTP_MAX_SERVERS; i++) {
      ip_addr_t sntp = *sntp_getserver(i);
      if (sntp.type == IPADDR_TYPE_V6) {
        Serial.printf("sntp%d:     is IPv6 only", i);
      }

      IPAddress sntpIP = sntp.u_addr.ip4.addr;
      const char* name = sntp_getservername(i);
      if (sntp.u_addr.ip4.addr != 0) {
        Serial.printf("sntp%d:     ", i);
        if (name) {
          Serial.printf("%s (%s) ", name, sntpIP.toString().c_str());
        } else {
          Serial.printf("%s ", sntpIP.toString().c_str());
        }
        Serial.printf("- Reachability: %o\n", sntp_getreachability(i));
      }
    }

  // Serial.println ("Manually setting the internal clock...");
  // https://github.com/fbiego/ESP32Time
    }
    */

  if (! HasTimeElapsed(&LastTimeClockSynced, 60*60*1000), false) {  // check every hour
    return;  // clock is already synced
  }

  configTime(0, 0, TIME_SERVER1, TIME_SERVER2, TIME_SERVER3);
  setenv("TZ", TIMEZONE, 1); // https://github.com/nayarsystems/posix_tz_db/

  Serial.print("NTP time sync");
  DisplayText ("NTP time sync");
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    DisplayText (".");
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  DisplayText ("\n");
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  DisplayText(asctime(&timeinfo), CLCYAN);
  }

/* https://cplusplus.com/reference/ctime/tm/
Member	Type	Meaning	Range
tm_sec	int	seconds after the minute	0-60*
tm_min	int	minutes after the hour	0-59
tm_hour	int	hours since midnight	0-23
tm_mday	int	day of the month	1-31
tm_mon	int	months since January	0-11
tm_year	int	years since 1900	
tm_wday	int	days since Sunday	0-6
tm_yday	int	days since January 1	0-365
tm_isdst	int	Daylight Saving Time flag	
*/

// global vars
int CurrentYear, CurrentMonth, CurrentWeekday, CurrentDay, CurrentHour, CurrentMinute;  

// fill global variables
bool GetCurrentTime(void) {
  struct tm timeinfo;
  bool result = getLocalTime(&timeinfo);
  CurrentYear  = timeinfo.tm_year + 1900;
  CurrentMonth = timeinfo.tm_mon + 1;
  CurrentDay   = timeinfo.tm_mday;
  CurrentHour  = timeinfo.tm_hour;
  CurrentMinute = timeinfo.tm_min;
  CurrentWeekday  = timeinfo.tm_wday; // days since Sunday	0-6
  if (CurrentWeekday == 0) CurrentWeekday = 7; // day of the week (1 = Mon, 2 = Tue,.. 7 = Sun)
  return result;  
}
