/*
 * Author: Aljaz Ogrin
 * Project: 
 * Hardware: ESP32
 * File description: Global configuration
 */
 
#ifndef __CONFIG_H_
#define __CONFIG_H_

// ************ General config *********************
#define DEBUG_OUTPUT
#define DEBUG_OUTPUT_DATA  // include received data
//#define DEBUG_IMG_TIMING   // image loading metrics and timing
//#define DEBUG_URNIK
//#define DISPLAY_TCP_MSGS

#define DEVICE_NAME "Display_v2"
#define WIFI_CONNECT_TIMEOUT_SEC 240  // How long to wait for WiFi

#define TIME_SERVER1  "si.pool.ntp.org"
#define TIME_SERVER2  "pool.ntp.org"
#define TIME_SERVER3  "time.nist.gov"
/* timezone: 
find your string here: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
decoding of the string: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
*/
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"  // Europe/Ljubljana

#define MORNING_TIME  6
#define DAY_TIME      7
#define EVENING_TIME 22
#define NIGHT_TIME   23
#define EVENING_TIME_BRIGHTNESS  20

// ************ WiFi config *********************
//#define WIFI_SSID  "..." -> enter into the file __CONFIG_SECRETS.h
//#define WIFI_PASSWD "..."
#define WIFI_RETRY_CONNECTION_SEC  90


// Client 
#define HEATPUMP_HOST "10.38.44.221" // toplotna
#define HEATPUMP_PORT  21212

#define SMOOTHIE_HOST "10.38.11.101" // 3D printer
#define SMOOTHIE_PORT  23

#define RADIO_URL "http://10.38.11.201/?status"

// Shelly
// DOC: https://shelly-api-docs.shelly.cloud/gen1/#shelly-3em-settings-emeter-index
#define SHELLY_3EM_HP_URL "http://10.38.22.63/status"  // Poraba TC
// DOC: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/EM#emgetstatus-example
#define SHELLY_3EM_ALL_URL "http://10.38.22.92/rpc/EM.GetStatus?id=0" // Poraba hise
// DOC: https://shelly-api-docs.shelly.cloud/gen2/Addons/ShellySensorAddon#sensoraddongetperipherals-example
// DOC: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Temperature#temperaturegetstatus-example
#define SHELLY_1PM_ADDON_URL "http://10.38.22.221/rpc/Temperature.GetStatus?id=101" // Bazen temperatura
// DOC: https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Switch#switchgetstatus-example
#define SHELLY_1PM_SW1_URL "http://10.38.22.221/rpc/Switch.GetStatus?id=0" // Bazen pumpa
#define SHELLY_1PM_SW2_URL "http://10.38.22.222/rpc/Switch.GetStatus?id=0" // Bazen toplotna

// ARSO:
#define ARSO_SERVER_CURRENT_XML_URL   "https://meteo.arso.gov.si/uploads/probase/www/observ/surface/text/sl/observation_LJUBL-ANA_BRNIK_latest.xml"
#define ARSO_SERVER_FORECAST_XML_URL  "https://meteo.arso.gov.si/uploads/probase/www/fproduct/text/sl/fcast_SI_OSREDNJESLOVENSKA_latest.xml"
#define ARSO_SERVER_METEOGRAM_XML_URL "https://meteo.arso.gov.si/uploads/probase/www/fproduct/text/sl/forecast_SI_OSREDNJESLOVENSKA_int3h.xml"
#define MTG_NUMPTS 25
#define ARSO_SERVER_RAIN_GIF_URL      "https://meteo.arso.gov.si/uploads/probase/www/observ/radar/si0_zm_pda_anim.gif"
// also validate: include\Arso_https_certificate.h


// ************ Hardware *********************

//#define USE_SD_CARD_SPI
#define USE_SD_CARD_MMC

#ifdef USE_SD_CARD_SPI
  #define SD_MOSI       23
  #define SD_MISO       19
  #define SD_SCK        18
  #define SD_CS          5
  #define IMAGES_ON_SD_CARD
#endif

#ifdef USE_SD_CARD_MMC
  #define SD_MMC_CMD    GPIO_NUM_38
  #define SD_MMC_CLK    GPIO_NUM_39
  #define SD_MMC_D0     GPIO_NUM_40
  #define IMAGES_ON_SD_CARD
#endif

  #define ST7796_DRIVER   // 4 inch LCD 480 x 320 

  // common TFT pins

  #define USE_HSPI_PORT  // for TFT display;  VSPI is used for SD card or MMC

  #define TFT_MISO      GPIO_NUM_9
  #define TFT_BL        GPIO_NUM_10
  #define TFT_SCLK      GPIO_NUM_11
  #define TFT_MOSI      GPIO_NUM_12
  #define TFT_DC        GPIO_NUM_13
  #define TFT_RST       -1 // connect to RESET pin / ESP32 EN pin
  #define TFT_CS        GPIO_NUM_14

//#define GIF_USE_DMA       // ESP32 ~1.25x single frame rendering performance boost for badgers.h
                            // Note: Do not use SPI DMA if reading GIF images from SPI SD card on same bus as TFT  

// ************ BODMER LIBRARY CONFIG *********************

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
//#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
//#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
//#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
//#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
//#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
//#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT
#define CONFIG_SPIRAM_SUPPORT 

#define SPI_FREQUENCY        20000000
#define SPI_READ_FREQUENCY   10000000
#define DISABLE_ALL_LIBRARY_WARNINGS
#define USER_SETUP_LOADED

// ************ SD CARD CONFIG *********************
#define MMC_FREQ_SDCARD   10000 // 10 MHz  SDMMC_FREQ_DEFAULT // 20 MHz
#define SPI_FREQ_SDCARD   20000000  // default 4 MHz; max 25 MHz
#define SPI_DMA_MAX 4095 

#endif /* __CONFIG_H_ */
