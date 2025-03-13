#include "__CONFIG.h"
#include <Arduino.h>

#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "display.h"
// Font files are stored in SPIFFS, so load the library
#include <SPIFFS.h>
#include <FS.h>
#include <SD.h>

signed int DspH;
signed int DspW;
uint16_t DspBgColor = TFT_BLACK;


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
 
  // PWM backlight
  const int freq = 567;  // Hz
  const int ledChannel = 9;  // channel < 8 -> speed_mode = 0 = LEDC_HIGH_SPEED_MODE
  const int resolution = 8; 


void DisplaySetBrightness(uint8_t Brightness) {
  // changing the LED brightness with PWM
  ledcWrite(ledChannel, Brightness);
}

void DisplayInit(void) {
  Serial.println("Starting LCD...");
  tft.init();
  #ifdef ST7789_DRIVER
  tft.invertDisplay(false);
  #endif
  tft.setRotation(3);
  tft.setTextWrap(true, true);
  DspH = tft.height();
  DspW = tft.width();
  Serial.printf("LCD size: %d x %d \n", DspW, DspH);
  DisplayClear();

  // configure LED PWM functionalitites
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html
  ledcSetup(ledChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(TFT_BL, ledChannel);
  // full power
  DisplaySetBrightness();
}

void DisplayInitFonts(void) {
  // extra fonts
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS available!");
  // ESP32 will crash if any of the fonts are missing; check if they are present on startup
  bool font_missing = false;
  if (SPIFFS.exists("/" + String(FONT_SIZE_15) + ".vlw")    == false) font_missing = true;
  if (SPIFFS.exists("/" + String(FONT_SIZE_20) + ".vlw")    == false) font_missing = true;
  if (SPIFFS.exists("/" + String(FONT_SIZE_24) + ".vlw")    == false) font_missing = true;
  if (SPIFFS.exists("/" + String(FONT_SIZE_28) + ".vlw")    == false) font_missing = true;
  if (SPIFFS.exists("/" + String(FONT_SIZE_36) + ".vlw")    == false) font_missing = true;

  if (font_missing)
  {
    Serial.println("\r\nFont missing in SPIFFS, did you upload it?");
    while(1) yield();
  }
  else Serial.println("\r\nFonts found OK.");
}


void DisplayClearCanvas(void) {
    DisplayClear();
}

void DisplayClear(uint16_t Color) {
  DspBgColor = Color;
  tft.unloadFont();
  tft.setCursor(0, 0);
  tft.fillScreen(Color);
  tft.setTextFont(1);
  tft.setCursor(0, 0);
  uint16_t TxtColor = TFT_WHITE;
  if (DspBgColor == TFT_WHITE) TxtColor = TFT_BLACK;
  tft.setTextColor(TxtColor, DspBgColor);
  tft.setTextDatum(TL_DATUM); // top left (default)
}

void DisplayText(const char Text[]) {
  tft.unloadFont();
  tft.setTextWrap(true, true);
  uint16_t TxtColor = TFT_WHITE;
  if (DspBgColor == TFT_WHITE) TxtColor = TFT_BLACK;
  tft.setTextColor(TxtColor, DspBgColor);
  tft.print(Text);
//  tft.drawString(Text, tft.getCursorX(), tft.getCursorY());
}

void DisplayText(const char Text[], uint16_t color) {
  tft.unloadFont();
  tft.setTextWrap(true, true);
  tft.setTextColor(color, DspBgColor);
  tft.print(Text);
//  tft.drawString(Text, tft.getCursorX(), tft.getCursorY());
}

  /*
  Font size 0 = 6x8 px
  Font size 1 = 24 px
  Font size 2 = 36 px
  */
void DisplayText(const char Text[], uint16_t FontSize, int16_t X, int16_t Y, uint16_t Color, bool Wrap) {
  tft.setTextWrap(Wrap, false);
  switch (FontSize)  {
  case 1:  tft.loadFont(FONT_SIZE_24);
    break;
  case 2:  tft.loadFont(FONT_SIZE_36);
    break;
  case 15:  tft.loadFont(FONT_SIZE_15);
    break;
  case 20:  tft.loadFont(FONT_SIZE_20);
    break;
  case 201:  tft.loadFont(FONT_SIZE_201);
    break;
  case 202:  tft.loadFont(FONT_SIZE_202);
    break;
  case 24:  tft.loadFont(FONT_SIZE_24);
    break;
  case 28:  tft.loadFont(FONT_SIZE_28);
    break;
  case 36:  tft.loadFont(FONT_SIZE_36);
    break;
  case 361:  tft.loadFont(FONT_SIZE_361);
    break;
  default: tft.unloadFont(); // Default small font
    break;
  }
  tft.setTextColor(Color, DspBgColor);
  tft.drawString(Text, X, Y);
}

void DisplayUpdate(void) {}


// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

/*
// Bodmer's BMP image rendering function
void DisplayShowImage_24bpp_only(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  if (!SPIFFS.exists(filename)) {
    Serial.print("File not found: ");
    Serial.println(filename);
    return;
  }

  fs::File bmpFS;

  // Open requested file
  bmpFS = SPIFFS.open(filename, FILE_READ);

  if (!bmpFS)
  {
    Serial.print("Error opening file: ");
    Serial.println(filename);
    return;
  }

  Serial.print("Loading: ");
  Serial.println(filename);

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t FileLineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(FileLineBuffer, sizeof(FileLineBuffer));
        uint8_t*  bptr = FileLineBuffer;
        uint16_t* tptr = (uint16_t*)FileLineBuffer;
        // Convert 24 to 16-bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)FileLineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}
*/

#ifdef IMAGES_ON_SD_CARD
  #define FILESYS  SD
#else
  #define FILESYS  SPIFFS
#endif

void DisplayShowImage(const char *filename, int16_t x, int16_t y, int16_t imgScaling) {
  uint32_t StartTime = millis();
  if ((x >= tft.width()) || (y >= tft.height())) return;

/*
#ifdef IMAGES_ON_SD_CARD
  if (imgScaling == 2) {
    String FN = filename;
    FN.remove(FN.indexOf("."));
    FN.concat("2.bmp");
    Serial.print("Searching for: ");
    Serial.print(FN);
    if (FILESYS.exists(FN)) {
      Serial.println("...Found");
      filename = FN.c_str();
    } else {
      Serial.println("...NOT found");
    }
  }
#endif
*/

/*
  // searching for a file on a SD card takes ages!
  if (!FILESYS.exists(filename)) {
    Serial.print("File not found: ");
    Serial.println(filename);
    return;
  }
*/
  fs::File bmpFS;

  // Open requested file
  bmpFS = FILESYS.open(filename, FILE_READ);

  if (!bmpFS)
  {
    Serial.print("Error opening file: ");
    Serial.println(filename);
    return;
  }

  Serial.print("Loading: ");
  Serial.println(filename);

#ifdef DEBUG_OUTPUT
  Serial.print("img file open time: ");
  Serial.println(millis() - StartTime);  
#endif

  uint32_t seekOffset, headerSize, paletteSize = 0;
  int16_t imgW, imgH, row, col, outW, outY;
  uint16_t  r, g, b, bitDepth;
  
  uint16_t magic = read16(bmpFS);
  if (magic == 0xFFFF) {
    Serial.print("Can't openfile. Make sure you upload the SPIFFs image with BMPs. : ");
    Serial.println(filename);
    bmpFS.close();
    return;
  }
  
  if (magic != 0x4D42) {
    Serial.print("File not a BMP. Magic: ");
    Serial.println(magic);
    bmpFS.close();
    return;
  }

  read32(bmpFS); // filesize in bytes
  read32(bmpFS); // reserved
  seekOffset = read32(bmpFS); // start of bitmap
  headerSize = read32(bmpFS); // header size
  imgW = read32(bmpFS); // width
  imgH = read32(bmpFS); // height
  read16(bmpFS); // color planes (must be 1)
  bitDepth = read16(bmpFS);
  
#ifdef DEBUG_OUTPUT
  Serial.print(" image W, H, BPP: ");
  Serial.print(imgW); 
  Serial.print(", "); 
  Serial.print(imgH);
  Serial.print(", "); 
  Serial.println(bitDepth);
#endif

  if (read32(bmpFS) != 0 || (bitDepth != 24 && bitDepth != 1 && bitDepth != 4 && bitDepth != 8)) {
    Serial.println("BMP format not recognized.");
    bmpFS.close();
    return;
  }

  // do not scale big images
  if (imgW > 33) {
    imgScaling = 1;
  }

  outY = y + (imgH * imgScaling) - 1; // draws from bottom up
  outW = imgW * imgScaling;

  bool oldSwapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);

  uint32_t palette[256];
  if (bitDepth <= 8) // 1,4,8 bit bitmap: read color palette
  {
    read32(bmpFS); read32(bmpFS); read32(bmpFS); // size, w resolution, h resolution
    paletteSize = read32(bmpFS);
    if (paletteSize == 0) paletteSize = bitDepth * bitDepth; // if 0, size is 2^bitDepth
    bmpFS.seek(14 + headerSize); // start of color palette
    for (uint16_t i = 0; i < paletteSize; i++) {
      palette[i] = read32(bmpFS);
    }
  }

  bmpFS.seek(seekOffset);

#ifdef DEBUG_OUTPUT
  Serial.print("img header processing time: ");
  Serial.println(millis() - StartTime);  
#endif

  uint32_t lineSize = ((bitDepth * imgW +31) >> 5) * 4;
  uint8_t FileLineBuffer[lineSize];
  uint16_t ImageLineBuffer[outW];

  // row is decremented as the BMP image is drawn bottom up
  for (row = imgH-1; row >= 0; row--) {
    bmpFS.read(FileLineBuffer, sizeof(FileLineBuffer));
    uint8_t*  bptr = FileLineBuffer;
    
    // Convert 24 to 16 bit colours while copying to output buffer.
    for (col = 0; col < imgW; col++) {
      if (bitDepth == 24) {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
        } else {
          uint32_t c = 0;
          if (bitDepth == 8) {
            c = palette[*bptr++];
          }
          else if (bitDepth == 4) {
            c = palette[(*bptr >> ((col & 0x01)?0:4)) & 0x0F];
            if (col & 0x01) bptr++;
          }
          else { // bitDepth == 1
            c = palette[(*bptr >> (7 - (col & 0x07))) & 0x01];
            if ((col & 0x07) == 0x07) bptr++;
          }
          b = c; g = c >> 8; r = c >> 16;
        }
      ImageLineBuffer[col*imgScaling]   = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xFF) >> 3);
      if (imgScaling == 2) {
      ImageLineBuffer[col*imgScaling+1] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xFF) >> 3);
      }
    } // col
    // Push the pixel row to screen, pushImage will crop the line if needed
    // y is decremented as the BMP image is drawn bottom up
    tft.pushImage(x, outY--, outW, 1, (uint16_t*)ImageLineBuffer);
    if (imgScaling == 2)
    tft.pushImage(x, outY--, outW, 1, (uint16_t*)ImageLineBuffer);
  } // row
  tft.setSwapBytes(oldSwapBytes); 
  bmpFS.close();
#ifdef DEBUG_OUTPUT
  Serial.print("img load time: ");
  Serial.println(millis() - StartTime);  
#endif
  return;
}



void DisplayFontTest(void) {

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set the font colour AND the background colour
                                          // so the anti-aliasing works

  tft.setCursor(0, 0); // Set cursor at top left of screen


  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Small font
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  tft.loadFont(FONT_SIZE_24); // Must load the font first

  tft.println("Small 15pt font"); // println moves cursor down for a new line

  tft.println(); // New line

  tft.print("ABC"); // print leaves cursor at end of line

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("1234"); // Added to line after ABC

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  // print stream formatting can be used,see:
  // https://www.arduino.cc/en/Serial/Print
  int ivalue = 1234;
  tft.println(ivalue);       // print as an ASCII-encoded decimal
  tft.println(ivalue, DEC);  // print as an ASCII-encoded decimal
  tft.println(ivalue, HEX);  // print as an ASCII-encoded hexadecimal
  tft.println(ivalue, OCT);  // print as an ASCII-encoded octal
  tft.println(ivalue, BIN);  // print as an ASCII-encoded binary

  tft.println(); // New line
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  float fvalue = 1.23456;
  tft.println(fvalue, 0);  // no decimal places
  tft.println(fvalue, 1);  // 1 decimal place
  tft.println(fvalue, 2);  // 2 decimal places
  tft.println(fvalue, 5);  // 5 decimal places

  delay(5000);

  // Get ready for the next demo while we have this font loaded
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0); // Set cursor at top left of screen
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Wrong and right ways to");
  tft.println("print changing values...");

  tft.unloadFont(); // Remove the font to recover memory used


  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Large font
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  tft.loadFont(FONT_SIZE_36); // Load another different font

  // Draw changing numbers - does not work unless a filled rectangle is drawn over the old text
  for (int i = 0; i <= 20; i++)
  {
    tft.setCursor(50, 50);
    tft.setTextColor(TFT_GREEN, TFT_BLACK); // TFT_BLACK is used for anti-aliasing only
                                            // By default background fill is off
    tft.print("      "); // Overprinting old number with spaces DOES NOT WORK!
    tft.setCursor(50, 50);
    tft.print(i / 10.0, 1);

    // Adding a parameter "true" to the setTextColor() function fills character background
    // This extra parameter is only for smooth fonts!
    tft.setTextColor(TFT_GREEN, TFT_BLACK, true);
    tft.setCursor(50, 90);
    tft.print(i / 10.0, 1);
    
    delay (200);
  }

  delay(5000);

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Large font text wrapping
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Change the font colour and the background colour

  tft.setCursor(0, 0); // Set cursor at top left of screen

  tft.println("Large font!");

  tft.setTextWrap(true); // Wrap on width
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("Long lines wrap to the next line");

  tft.setTextWrap(false, false); // Wrap on width and height switched off
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.println("Unless text wrap is switched off");

  tft.unloadFont(); // Remove the font to recover memory used

  delay(8000);

}








/*
 Font draw speed and flicker test, draws all numbers 0-999 in each font
 (0-99 in font 8)
 Average time in milliseconds to draw a character is shown in red
 A total of 2890 characters are drawn in each font (190 in font 8)
 
 Needs fonts 2, 4, 6, 7 and 8

 Make sure all the display driver and pin connections are correct by
 editing the User_Setup.h file in the TFT_eSPI library folder.

 Note that yield() or delay(0) must be called in long duration for/while
 loops to stop the ESP8266 watchdog triggering.
 */
unsigned long drawTime = 0;

void DisplayTest(void) {

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    tft.drawNumber(i, 0, 0, 1);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);
 
  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    tft.drawNumber(i, 0, 0, 2);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    tft.drawNumber(i, 0, 0, 4);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield(); tft.drawNumber(i, 0, 0, 6);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);
 
  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield(); tft.drawNumber(i, 0, 0, 7);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 100; i++) {
    yield(); tft.drawNumber(i, 0, 0, 8);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 190.0, 3, 0, 80, 4);

  delay(4000);
}


