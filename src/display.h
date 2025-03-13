#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include "__CONFIG.h"

  #include <TFT_eSPI.h>
  #define CLWHITE     TFT_WHITE
  #define CLORANGE    TFT_ORANGE
  #define CLRED       TFT_RED
  #define CLGREEN     TFT_GREEN
  #define CLBLUE      TFT_BLUE
  #define CLYELLOW    TFT_YELLOW
  #define CLCYAN      TFT_CYAN
  #define CLGREY      TFT_DARKGREY
  #define CLBLACK     TFT_BLACK
  #define CLLIGHTBLUE 0x33FF
  #define CLDARKBLUE  0x000F         // 0x001F -> 0F
  #define CLDARKGREEN TFT_DARKGREEN
  #define CLDARKGREY  0x39E7      /* 64, 64, 64 */
  #define CLLIGHTPINK 0xFC9F   // 11111  100100  11111   FF 90 FF
  #define CLPINK      0xF09F   // 11110  000100  11111   F7 00 FF
  #define CLLIGHTCYAN 0x4FFF

//  The fonts used are in the data folder and loaded onto SPIFFS.

//  A processing sketch to create new fonts can be found in the Tools folder of TFT_eSPI
//  https://github.com/Bodmer/TFT_eSPI/tree/master/Tools/Create_Smooth_Font/Create_font

//  This sketch uses font files created from the Noto family of fonts:
//  https://www.google.com/get/noto/

//0 -> Original Adafruit 8 pixel font 
#define FN_TXT_SMALL    "15-Noto-Sans-Bold"
#define FN_URNIK_TT     "20-MVBoli" // "20-KristenITC"
#define FN_URNIK_MM     "20-Ebrima-Bold"
#define FN_TXT          "22-SegoeUI" // 20-Noto-Sans-Mono 
#define FN_TITLE        "30-SegoeUI-Bold"
#define FN_TEMP_METEO   "40-Rockwell-ExtraBold"
#define FN_TEMP_SINGLE  "60-Rockwell-ExtraBold"

enum FontSize_t {
  FONT_SYS,
  FONT_TXT_SMALL,
  FONT_URNIK_TT,
  FONT_URNIK_MM,
  FONT_TXT,
  FONT_TITLE,
  FONT_TEMP_METEO,
  FONT_TEMP_SINGLE
};

  extern TFT_eSPI tft; // for graphical plot

  extern signed int DspH;
  extern signed int DspW;

  void DisplayInit(void);
  void DisplayInitFonts(void);
  void DisplaySetBrightness(uint8_t Brightness = 255);
  void DisplayClear(uint16_t Color = TFT_BLACK);
  void DisplayText(const char Text[]);
  void DisplayText(const char Text[], uint16_t color);
  void DisplayText(const char Text[], FontSize_t FontSize, int16_t X, int16_t Y, uint16_t Color = 0xFFFF, bool Wrap=false);
  void DisplayUpdate(void);

  void DisplayShowImage(const char *filename, int16_t x, int16_t y, int16_t imgScaling = 1);


  void DisplayTest(void);
  void DisplayFontTest(void);

#endif // __DISPLAY_H_