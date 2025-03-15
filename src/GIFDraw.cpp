#include <Arduino.h>
#include <AnimatedGIF.h>
#include "__CONFIG.h"
#include "display.h"
#include "GIFDraw.h"

// GIFDraw is called by AnimatedGIF library frame to screen

#ifdef GIF_USE_DMA
uint16_t usTemp[2][GIF_BUFFER_SIZE]; // Global to support DMA use
#else
uint16_t usTemp[1][GIF_BUFFER_SIZE]; // Global to support DMA use
#endif
bool dmaBuf = 0;

AnimatedGIF gif;

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
      while (c != ucTransparent && s < pEnd && iCount < GIF_BUFFER_SIZE)
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
        tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
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
    if (iWidth <= GIF_BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++)
        usTemp[dmaBuf][iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < GIF_BUFFER_SIZE; iCount++)
        usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef GIF_USE_DMA // 71.6 fps (ST7796 84.5 fps)
    tft.dmaWait();
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
#endif

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= GIF_BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++)
          usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < GIF_BUFFER_SIZE; iCount++)
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

bool DisplayGIF(uint8_t *imageData, int imageSize, uint8_t numberRepetitons)
{
  bool result = false;
  int rr = 0;
  if (gif.open((uint8_t *)imageData, imageSize, GIFrenderToLCD_callback))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    tft.startWrite(); // The TFT chip select is locked low
    for (uint8_t i = 0; i < numberRepetitons; i++)
    {
      // it automatically restarts the gif
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


void initGIF(void)
{
  gif.begin(BIG_ENDIAN_PIXELS);
}

