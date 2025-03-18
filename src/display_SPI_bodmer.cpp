#include "__CONFIG.h"
#include <Arduino.h>

//#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "display.h"
// Font files are stored in SPIFFS, so load the library
#include <SPIFFS.h>
#include <FS.h>
#include "SD_Card.h"

signed int DspH;
signed int DspW;
uint16_t DspBgColor = TFT_BLACK;

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

// PWM backlight
const int freq = 200; // Hz
const int ledChannel = 1;

void DisplaySetBrightness(uint8_t Brightness)
{
  // changing the LED brightness with PWM
  ledcWrite(ledChannel, Brightness);
}

// preload fonts to RAM
char * RAM_FONT_TEMP_METEO = nullptr;
char * RAM_FONT_TEMP_SINGLE = nullptr;

void DisplayInit(void)
{
  Serial.println("Starting LCD...");
  // PWM backlight
  // configure LED PWM functionalitites
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html
  ledcSetup(ledChannel, freq, 8); // 8-bit resolution for LEDC channel
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(TFT_BL, ledChannel);
  DisplaySetBrightness(0); // baklight off

  tft.init();
#ifdef GIF_USE_DMA
  tft.initDMA();
#endif
  tft.setRotation(3);
  tft.setTextWrap(true, true);
  DspH = tft.height();
  DspW = tft.width();
  Serial.printf("LCD size: %d x %d \r\n", DspW, DspH);
  DisplayClear();

  tft.drawPixel(2, 2, TFT_DARKGREY);
  Serial.print(" Pixel value written = ");
  Serial.println(TFT_DARKGREY, HEX);
  uint32_t rd = tft.readPixel(2, 2);
  Serial.print(" Pixel value read    = ");
  Serial.println(rd, HEX);

  if (rd != TFT_DARKGREY)
  {
    Serial.println("TFT comm error!");
    // while(1) yield();
  }
  else
    Serial.println(" TFT comm OK ");

  // full power
  DisplaySetBrightness();
}

void DisplayInitFonts(void)
{
  // extra fonts
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS available!");
  // ESP32 will crash if any of the fonts are missing; check if they are present on startup
  bool font_missing = false;
  if (SPIFFS.exists("/" + String(FN_TXT_SMALL) + ".vlw") == false)
    font_missing = true;
  if (SPIFFS.exists("/" + String(FN_TXT) + ".vlw") == false)
    font_missing = true;
  if (SPIFFS.exists("/" + String(FN_URNIK_TT) + ".vlw") == false)
    font_missing = true;
  if (SPIFFS.exists("/" + String(FN_URNIK_MM) + ".vlw") == false)
    font_missing = true;
  if (SPIFFS.exists("/" + String(FN_TITLE) + ".vlw") == false)
    font_missing = true;
  if (SPIFFS.exists("/" + String(FN_TEMP_METEO) + ".vlw") == false)
    font_missing = true;
  if (SPIFFS.exists("/" + String(FN_TEMP_SINGLE) + ".vlw") == false)
    font_missing = true;

  if (font_missing)
  {
    Serial.println("\r\nFont missing in SPIFFS, did you upload it?");
    DisplayText("Font(s) missing in SPIFFS!\r\n", CLRED);
    while (1)
      yield();
  }
  else
  {
    Serial.println("\r\nFonts found OK.");
    DisplayText("Fonts found ok.\r\n", CLGREEN);
    // preload fonts to RAM
    size_t fontFileSize;
    bool ok = false;

    fontFileSize = GetFileSize_SD(FN_TEMP_SINGLE_SD);
    if (fontFileSize > 0)
    {
      RAM_FONT_TEMP_SINGLE = (char *)ps_malloc(fontFileSize);
      if (RAM_FONT_TEMP_SINGLE != NULL)
      {
        ok = loadFileFromSDcardToMerory(FN_TEMP_SINGLE_SD, RAM_FONT_TEMP_SINGLE, fontFileSize, false);
      }
    }

    fontFileSize = GetFileSize_SD(FN_TEMP_METEO_SD);
    if (fontFileSize > 0)
    {
      RAM_FONT_TEMP_METEO = (char *)ps_malloc(fontFileSize);
      if (RAM_FONT_TEMP_METEO != NULL)
      {
        ok = loadFileFromSDcardToMerory(FN_TEMP_METEO_SD, RAM_FONT_TEMP_METEO, fontFileSize, false);
      }
    }
  
    if (!ok)
    {
      Serial.println("Error loading fonts from SD card to RAM!");
      DisplayText("Error loading fonts from SD card to RAM!\r\n", CLRED);
      while (1)
        yield();
  
    }
  }
}

void DisplayClear(uint16_t Color)
{
  DspBgColor = Color;
  tft.unloadFont();
  tft.setCursor(0, 0);
  tft.fillScreen(Color);
  tft.setTextFont(1); // default system font
  tft.setCursor(0, 0);
  uint16_t TxtColor = TFT_WHITE;
  if (DspBgColor == TFT_WHITE)
    TxtColor = TFT_BLACK;
  tft.setTextColor(TxtColor, DspBgColor);
  tft.setTextDatum(TL_DATUM); // top left (default)
}

void DisplayText(const char Text[])
{
  tft.unloadFont();
  tft.setTextWrap(true, true);
  uint16_t TxtColor = TFT_WHITE;
  if (DspBgColor == TFT_WHITE)
    TxtColor = TFT_BLACK;
  tft.setTextColor(TxtColor, DspBgColor);
  tft.print(Text);
  //  tft.drawString(Text, tft.getCursorX(), tft.getCursorY());
}

void DisplayText(const char Text[], uint16_t color)
{
  tft.unloadFont();
  tft.setTextWrap(true, true);
  tft.setTextColor(color, DspBgColor);
  tft.print(Text);
  //  tft.drawString(Text, tft.getCursorX(), tft.getCursorY());
}

void DisplayText(const char Text[], FontSize_t FontSize, int16_t X, int16_t Y, uint16_t Color, bool Wrap)
{
  tft.setTextWrap(Wrap, false);
  switch (FontSize)
  {
  case FONT_TXT_SMALL:
    tft.loadFont(FN_TXT_SMALL); // loads parameters from a font vlw file
    break;
  case FONT_TXT:
    tft.loadFont(FN_TXT);
    break;
  case FONT_URNIK_TT:
    tft.loadFont(FN_URNIK_TT);
    break;
  case FONT_URNIK_MM:
    tft.loadFont(FN_URNIK_MM);
    break;
  case FONT_TITLE:
    tft.loadFont(FN_TITLE);
    break;
  case FONT_TEMP_METEO:
    //tft.loadFont(FN_TEMP_METEO);
    tft.loadFont((uint8_t *)RAM_FONT_TEMP_METEO);
    break;
  case FONT_TEMP_SINGLE:
    //tft.loadFont(FN_TEMP_SINGLE);
    tft.loadFont((uint8_t *)RAM_FONT_TEMP_SINGLE);
    break;
  default:
    tft.unloadFont(); // Default small font
    break;
  }
  tft.setTextColor(Color, DspBgColor);
  tft.drawString(Text, X, Y);
}

void DisplayUpdate(void) {}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}


void DisplayShowImage(const char *filename, int16_t x, int16_t y, int16_t imgScaling)
{
  uint32_t StartTime = millis();
  if ((x >= tft.width()) || (y >= tft.height()))
    return;

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

#ifdef DEBUG_OUTPUT
  Serial.print("Loading: ");
  Serial.println(filename);
#endif

#ifdef DEBUG_IMG_TIMING
  Serial.print("img file open time: ");
  Serial.println(millis() - StartTime);
#endif

  uint32_t seekOffset, headerSize, paletteSize = 0;
  int16_t imgW, imgH, row, col, outW, outY;
  uint16_t r, g, b, bitDepth;

  uint16_t magic = read16(bmpFS);
  if (magic == 0xFFFF)
  {
    Serial.print("Can't openfile. Make sure you upload the SPIFFs image with BMPs. : ");
    Serial.println(filename);
    bmpFS.close();
    return;
  }

  if (magic != 0x4D42)
  {
    Serial.print("File not a BMP. Magic: ");
    Serial.println(magic);
    bmpFS.close();
    return;
  }

  read32(bmpFS);              // filesize in bytes
  read32(bmpFS);              // reserved
  seekOffset = read32(bmpFS); // start of bitmap
  headerSize = read32(bmpFS); // header size
  imgW = read32(bmpFS);       // width
  imgH = read32(bmpFS);       // height
  read16(bmpFS);              // color planes (must be 1)
  bitDepth = read16(bmpFS);

#ifdef DEBUG_IMG_TIMING
  Serial.print(" image W, H, BPP: ");
  Serial.print(imgW);
  Serial.print(", ");
  Serial.print(imgH);
  Serial.print(", ");
  Serial.println(bitDepth);
#endif

  if (read32(bmpFS) != 0 || (bitDepth != 24 && bitDepth != 1 && bitDepth != 4 && bitDepth != 8))
  {
    Serial.println("BMP format not recognized.");
    bmpFS.close();
    return;
  }

  // do not scale big images
  if (imgW > 33)
  {
    imgScaling = 1;
  }

  outY = y + (imgH * imgScaling) - 1; // draws from bottom up
  outW = imgW * imgScaling;

  bool oldSwapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);

  uint32_t palette[256];
  if (bitDepth <= 8) // 1,4,8 bit bitmap: read color palette
  {
    read32(bmpFS);
    read32(bmpFS);
    read32(bmpFS); // size, w resolution, h resolution
    paletteSize = read32(bmpFS);
    if (paletteSize == 0)
      paletteSize = bitDepth * bitDepth; // if 0, size is 2^bitDepth
    bmpFS.seek(14 + headerSize);         // start of color palette
    for (uint16_t i = 0; i < paletteSize; i++)
    {
      palette[i] = read32(bmpFS);
    }
  }

  bmpFS.seek(seekOffset);

#ifdef DEBUG_IMG_TIMING
  Serial.print("img header processing time: ");
  Serial.println(millis() - StartTime);
#endif

  uint32_t lineSize = ((bitDepth * imgW + 31) >> 5) * 4;
  uint8_t FileLineBuffer[lineSize];
  uint16_t ImageLineBuffer[outW];

  // row is decremented as the BMP image is drawn bottom up
  for (row = imgH - 1; row >= 0; row--)
  {
    bmpFS.read(FileLineBuffer, sizeof(FileLineBuffer));
    uint8_t *bptr = FileLineBuffer;

    // Convert 24 to 16 bit colours while copying to output buffer.
    for (col = 0; col < imgW; col++)
    {
      if (bitDepth == 24)
      {
        b = *bptr++;
        g = *bptr++;
        r = *bptr++;
      }
      else
      {
        uint32_t c = 0;
        if (bitDepth == 8)
        {
          c = palette[*bptr++];
        }
        else if (bitDepth == 4)
        {
          c = palette[(*bptr >> ((col & 0x01) ? 0 : 4)) & 0x0F];
          if (col & 0x01)
            bptr++;
        }
        else
        { // bitDepth == 1
          c = palette[(*bptr >> (7 - (col & 0x07))) & 0x01];
          if ((col & 0x07) == 0x07)
            bptr++;
        }
        b = c;
        g = c >> 8;
        r = c >> 16;
      }
      ImageLineBuffer[col * imgScaling] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xFF) >> 3);
      if (imgScaling == 2)
      {
        ImageLineBuffer[col * imgScaling + 1] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xFF) >> 3);
      }
    } // col
    // Push the pixel row to screen, pushImage will crop the line if needed
    // y is decremented as the BMP image is drawn bottom up
    tft.pushImage(x, outY--, outW, 1, (uint16_t *)ImageLineBuffer);
    if (imgScaling == 2)
      tft.pushImage(x, outY--, outW, 1, (uint16_t *)ImageLineBuffer);
  } // row
  tft.setSwapBytes(oldSwapBytes);
  bmpFS.close();
#ifdef DEBUG_IMG_TIMING
  Serial.print("img load time: ");
  Serial.println(millis() - StartTime);
#endif
  return;
}

void DisplayFontTest(void)
{
  DisplayClear();
  DisplayText(FN_TXT_SMALL,   FONT_TXT_SMALL,   20,  20, CLWHITE);
  DisplayText(FN_URNIK_TT,    FONT_URNIK_TT,    20,  40, CLWHITE);
  DisplayText(FN_URNIK_MM,    FONT_URNIK_MM,    20,  70, CLWHITE);
  DisplayText(FN_TXT,         FONT_TXT,         20, 100, CLWHITE);
  DisplayText(FN_TITLE,       FONT_TITLE,       20, 130, CLWHITE);
  DisplayText(FN_TEMP_METEO,  FONT_TEMP_METEO,  20, 170, CLWHITE);
  DisplayText(FN_TEMP_SINGLE, FONT_TEMP_SINGLE, 20, 220, CLWHITE);

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

void DisplayTest(void)
{

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  drawTime = millis();

  for (int i = 0; i < 1000; i++)
  {
    tft.drawNumber(i, 0, 0, 1);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++)
  {
    tft.drawNumber(i, 0, 0, 2);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++)
  {
    tft.drawNumber(i, 0, 0, 4);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++)
  {
    yield();
    tft.drawNumber(i, 0, 0, 6);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++)
  {
    yield();
    tft.drawNumber(i, 0, 0, 7);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 2890.0, 3, 0, 80, 4);

  delay(4000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawTime = millis();

  for (int i = 0; i < 100; i++)
  {
    yield();
    tft.drawNumber(i, 0, 0, 8);
  }

  drawTime = millis() - drawTime;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawFloat(drawTime / 190.0, 3, 0, 80, 4);

  delay(4000);
}
