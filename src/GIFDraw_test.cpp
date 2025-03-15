// TFT_eSPI_memory
//
// Example sketch which shows how to display an
// animated GIF image stored in FLASH memory
//
// written by Larry Bank
// bitbank@pobox.com
//
// Adapted by Bodmer for the TFT_eSPI Arduino library:
// https://github.com/Bodmer/TFT_eSPI
//
// To display a GIF from memory, a single callback function
// must be provided - GIFrenderToLCD_callback
// This function is called after each scan line is decoded
// and is passed the 8-bit pixels, RGB565 palette and info
// about how and where to display the line. The palette entries
// can be in little-endian or big-endian order; this is specified
// in the begin() method.
//
// The AnimatedGIF class doesn't allocate or free any memory, but the
// instance data occupies about 22.5K of RAM.

#include <Arduino.h>
#include "__CONFIG.h"
#include "display.h"
#include "GlobalVariables.h"
// Load GIF library
#include <AnimatedGIF.h>
#include "GIFDraw.h"

//AnimatedGIF gif;

// Example AnimatedGIF library images
#include "../DOC/GIF/badgers.h"
#include "../DOC/GIF/pattern.h"

// ESP32 40MHz SPI single frame rendering performance
// Note: no DMA performance gain on smaller images or transparent pixel GIFs
#define GIF_IMAGE badgers //  No DMA  63 fps, DMA:  71fps
//#define GIF_IMAGE pattern   //  No DMA  90 fps, DMA:  78 fps

/*
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
*/
void gif_draw_test()
{
//  tft.begin();
#ifdef GIF_USE_DMA
  tft.initDMA();
#endif
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  //gif.begin(BIG_ENDIAN_PIXELS);

  while (1)
  {

#ifdef GIF_SPEED_TEST  // Test maximum rendering speed 

long lTime = micros();
int iFrames = 0;

if (gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFrenderToLCD_callback))
{
  tft.startWrite(); // For DMA the TFT chip select is locked low
  while (gif.playFrame(false, NULL))
  {
    // Each loop renders one frame
    iFrames++;
    yield();
  }
  gif.close();
  tft.endWrite(); // Release TFT chip select for other SPI devices
  lTime = micros() - lTime;
  Serial.print(iFrames / (lTime / 1000000.0));
  Serial.println(" fps");
}

#else  // Render at rate that is GIF controlled

/*
if (gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFrenderToLCD_callback))
{
  Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
  tft.startWrite(); // The TFT chip select is locked low
  while (gif.playFrame(true, NULL))
  {
    yield();
  }
  gif.close();
  tft.endWrite(); // Release TFT chip select for other SPI devices
}
*/

  Serial.println("drawing image 10x");
  memcpy(&GIFimage, &GIF_IMAGE, sizeof(GIF_IMAGE));
  DisplayGIF(GIFimage, sizeof(GIF_IMAGE), 10);


#endif
  }  // inf while loop
}
