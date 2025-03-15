

//#define GIF_SPEED_TEST


#define GIF_DISPLAY_WIDTH  DspW
#define GIF_DISPLAY_HEIGHT DspH
#define GIF_BUFFER_SIZE 256            // Optimum is >= GIF width or integral division of width

//void GIFrenderToLCD_callback(GIFDRAW *pDraw);

bool DisplayGIF(uint8_t *imageData, int imageSize, uint8_t numberRepetitons, int positionX = -1, int positionY = -1);
void initGIF(void);
