
bool SDcardInit(void);
size_t GetFileSize_SD(const char *filename);
bool loadFileFromSDcardToMerory(const char *filename, char *buffer, size_t maxSize, bool NullTerminate);

void SD_TEST(void);

#if defined(USE_SD_CARD_SPI)
    #include <SD.h>
    #include <SPI.h>
    #define  FILESYS SD
#elif defined(USE_SD_CARD_MMC)
    #include <SD_MMC.h>
    #define  FILESYS SD_MMC
#else
    #define FILESYS SPIFFS
#endif
