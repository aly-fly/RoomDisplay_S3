

//#define GIF_SPEED_TEST

#define GIF_DIRECT_RENDER_TO_LCD
#define GIF_SCALING      1

#define GIF_IMG_WIDTH    220
#define GIF_IMG_HEIGHT   208

#define GIF_DISPLAY_WIDTH  DspW
#define GIF_DISPLAY_HEIGHT DspH
#define GIF_LINE_BUFFER_SIZE  GIF_IMG_WIDTH            // Optimum is >= GIF width or integral division of width

bool DisplayGIF(uint8_t *imageData, int imageSize, uint8_t numberRepetitons, int positionX = -1, int positionY = -1);
void initGIF(void);
