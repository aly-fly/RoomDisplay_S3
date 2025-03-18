#include <Arduino.h>
#include <AnimatedGIF.h>
#include "__CONFIG.h"
#include "display.h"
#include "GIFDraw.h"

AnimatedGIF gif;

void initGIF(void)
{
  gif.begin(BIG_ENDIAN_PIXELS);
}

#ifdef GIF_DIRECT_RENDER_TO_LCD

// DIRECT RENDER TO TFT DISPLAY
// Code is from the example:
// https://github.com/bitbank2/AnimatedGIF/tree/master/examples/TFT_eSPI_memory

#ifdef GIF_USE_DMA
uint16_t usTemp[2][GIF_LINE_BUFFER_SIZE]; // Global to support DMA use
#else
uint16_t usTemp[1][GIF_LINE_BUFFER_SIZE]; // Global to support DMA use
#endif
bool dmaBuf = 0;

int iOffX = 0, iOffY = 0;

// Draw a line of image directly on the LCD
void GIFrenderToLCD_callback(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

  // Display bounds check and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > GIF_DISPLAY_WIDTH)
    iWidth = GIF_DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= GIF_DISPLAY_HEIGHT || pDraw->iX >= GIF_DISPLAY_WIDTH || iWidth < 1)
    return;

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0][0];
      while (c != ucTransparent && s < pEnd && iCount < GIF_LINE_BUFFER_SIZE)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        tft.setAddrWindow(pDraw->iX + x + iOffX, y + iOffY, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= GIF_LINE_BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++)
        usTemp[dmaBuf][iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < GIF_LINE_BUFFER_SIZE; iCount++)
        usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef GIF_USE_DMA // 71.6 fps (ST7796 84.5 fps)
    tft.dmaWait();
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
    tft.setAddrWindow(pDraw->iX + iOffX, y + iOffY, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
#endif

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= GIF_LINE_BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++)
          usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < GIF_LINE_BUFFER_SIZE; iCount++)
          usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef GIF_USE_DMA
      tft.dmaWait();
      tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
      dmaBuf = !dmaBuf;
#else
      tft.pushPixels(&usTemp[0][0], iCount);
#endif
      iWidth -= iCount;
    }
  }
} /* GIFDraw() */

// ======================================================================================================
// ======================================================================================================

bool DisplayGIF(uint8_t *imageData, int imageSize, uint8_t numberRepetitons, int positionX, int positionY)
{
  bool result = false;
  int rr = 0;
  if (gif.open((uint8_t *)imageData, imageSize, GIFrenderToLCD_callback))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    if (positionX == -1) // center on the display
    {
      iOffX = (GIF_DISPLAY_WIDTH - gif.getCanvasWidth()) / 2;
      iOffY = (GIF_DISPLAY_HEIGHT - gif.getCanvasHeight()) / 2;
    }
    else
    {
      iOffX = positionX;
      iOffY = positionY;
    }
    tft.startWrite(); // The TFT chip select is locked low
    for (uint8_t i = 0; i < numberRepetitons; i++)
    {
      // it automatically restarts the gif from the beginning
      while (rr = gif.playFrame(true, NULL))
      {
        yield();
      }
      if (rr >= 0)
      {
        result = true;
      }
    }
    gif.close();
    tft.endWrite(); // Release TFT chip select for other SPI devices
  }
  return result;
} // DisplayGIF()

// ======================================================================================================
// ======================================================================================================

#else // not GIF_DIRECT_RENDER_TO_LCD

// RENDER TO MEMORY -> RESIZE -> TFT DISPLAY
// Code is from the example:
// https://github.com/bitbank2/AnimatedGIF/blob/master/examples/big_mem_demo/big_mem_demo.ino

// The code for MCUs with enough RAM is much simpler because the AnimatedGIF library can
// generate "cooked" pixels that are ready to send to the display

// Draw a line of image into the buffer
void GIFrenderToLCD_callback(GIFDRAW *pDraw)
{
  if (pDraw->y == 0)
  { // set the memory window when the first line is rendered
    tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
  }
  // For all other lines, just push the pixels to the display
  tft.pushPixels((uint8_t *)pDraw->pPixels, pDraw->iWidth * 2);
}

// memory allocation callback function
void *GIFAlloc(uint32_t u32Size)
{
  return malloc(u32Size);
}

// memory free callback function
void GIFFree(void *p)
{
  free(p);
}

/*
#define iCanvasSize  (GIF_IMG_WIDTH * (GIF_IMG_HEIGHT + 3))
__attribute__((section(".ext_ram.bss")))  uint8_t imageStaticCanvas[iCanvasSize];
*/

bool DisplayGIF(uint8_t *imageData, int imageSize, uint8_t numberRepetitons, int positionX, int positionY)
{
  bool ok = false;
  bool result = false;
  int rr = 0;
  if (gif.open((uint8_t *)imageData, imageSize, GIFrenderToLCD_callback))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    // Allocate an internal buffer to hold the full canvas size

#ifdef iCanvasSize
    gif.setFrameBuf(imageStaticCanvas);
    ok = (gif.getCanvasWidth() == GIF_IMG_WIDTH) && (gif.getCanvasHeight() == GIF_IMG_HEIGHT);
#else
    ok = (gif.allocFrameBuf(GIFAlloc) == GIF_SUCCESS);
#endif

    if (ok)
    {
      gif.setDrawType(GIF_DRAW_COOKED); // we want the library to generate ready-made pixels
      tft.startWrite();                 // The TFT chip select is locked low
      for (uint8_t i = 0; i < numberRepetitons; i++)
      {
        // it automatically restarts the gif from the beginning
        while (rr = gif.playFrame(true, NULL))
        {
          yield();
        }
        if (rr >= 0)
        {
          result = true;
        }
      }
      gif.close();
      tft.endWrite(); // Release TFT chip select for other SPI devices
#ifndef iCanvasSize
      gif.freeFrameBuf(GIFFree);
#endif
    }
    else
    {
      Serial.println("Insufficient memory!");
    }
    gif.close();
  }
  return result;
} // DisplayGIF()

#endif // GIF_DIRECT_RENDER_TO_LCD
