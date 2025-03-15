
// Format the microSD card as FAT32.
// Make sure power supply is 3.3 V

// Using the SPI controller.
// https://github.com/espressif/arduino-esp32/tree/master/libraries/SD

// If you use the SDD_MMC library you’re using the ESP32 SD/SDIO/MMC controller.
// doc: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/sdmmc.html
// lib: https://github.com/espressif/arduino-esp32/tree/master/libraries/SD_MMC

// examples: https://github.com/espressif/esp-idf/tree/46acfdce/examples/storage/sd_card

/*
 * pin 1 - not used          |  Micro SD card     |
 * pin 2 - CS (SS)           |                   /
 * pin 3 - DI (MOSI)         |                  |__
 * pin 4 - VDD (3.3V)        |                    |
 * pin 5 - SCK (SCLK)        | 8 7 6 5 4 3 2 1   /
 * pin 6 - VSS (GND)         | ▄ ▄ ▄ ▄ ▄ ▄ ▄ ▄  /
 * pin 7 - DO (MISO)         | ▀ ▀ █ ▀ █ ▀ ▀ ▀ |
 * pin 8 - not used          |_________________|
 *                             ║ ║ ║ ║ ║ ║ ║ ║
 *                     ╔═══════╝ ║ ║ ║ ║ ║ ║ ╚═════════╗
 *                     ║         ║ ║ ║ ║ ║ ╚══════╗    ║
 *                     ║   ╔═════╝ ║ ║ ║ ╚═════╗  ║    ║
 * Connections for     ║   ║   ╔═══╩═║═║═══╗   ║  ║    ║
 * full-sized          ║   ║   ║   ╔═╝ ║   ║   ║  ║    ║
 * SD card             ║   ║   ║   ║   ║   ║   ║  ║    ║
 * Pin name         |  -  DO  VSS SCK VDD VSS DI CS    -  |
 * SD pin number    |  8   7   6   5   4   3   2   1   9 /
 *                  |                                  █/
 *                  |__█___█___█___█___█___█___█___█___/
 *
 * Note:  The SPI pins can be manually configured by using `SPI.begin(sck, miso, mosi, cs).`
 *        Alternatively, you can change the CS pin and use the other default settings by using `SD.begin(cs)`.
 *
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SPI Pin Name | ESP8266 | ESP32 | ESP32‑S2 | ESP32‑S3 | ESP32‑C3 | ESP32‑C6 | ESP32‑H2 |
 * +==============+=========+=======+==========+==========+==========+==========+==========+
 * | CS (SS)      | GPIO15  | GPIO5 | GPIO34   | GPIO10   | GPIO7    | GPIO18   | GPIO0    |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DI (MOSI)    | GPIO13  | GPIO23| GPIO35   | GPIO11   | GPIO6    | GPIO19   | GPIO25   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DO (MISO)    | GPIO12  | GPIO19| GPIO37   | GPIO13   | GPIO5    | GPIO20   | GPIO11   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SCK (SCLK)   | GPIO14  | GPIO18| GPIO36   | GPIO12   | GPIO4    | GPIO21   | GPIO10   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 *
 * For more info see file README.md in this library or on URL:
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
 */

#include "__CONFIG.h"
#include <Arduino.h>
#include <FS.h>
#include "SD_Card.h"


// ===========================================================================================================================

bool SDcardInit(void)
{
#ifdef IMAGES_ON_SD_CARD

#ifdef USE_SD_CARD_SPI
  // Default = VSPI
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, SPI_FREQ_SDCARD))
  {
    Serial.println("SPI SD Card Mount Failed");
    return false;
  }
  Serial.println("SPI SD Card Mount OK");
#elif defined(USE_SD_CARD_MMC)
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5))
  {
    Serial.println("MMC SD Card Mount Failed");
    return false;
  }
  Serial.println("MMC SD Card Mount OK");
#endif

  uint8_t cardType = FILESYS.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("Uknown SD Card Type!");
    return false;
  }

  Serial.print("  SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }

  Serial.printf("  SD Card Size: %llu MB\n", FILESYS.cardSize() / (1024 * 1024));
  Serial.printf("  Total space: %llu MB\n", FILESYS.totalBytes() / (1024 * 1024));
  Serial.printf("  Used space: %llu MB\n", FILESYS.usedBytes() / (1024 * 1024));

  return true;
#else
  return false;
#endif
}

// ===========================================================================================================================
// ===========================================================================================================================

bool loadFileFromSDcardToMerory(const char *filename, char *buffer, size_t maxSize)
{
  Serial.printf("Reading file: %s\n", filename);
  File file = FILESYS.open(filename);
  if (!file)
  {
    Serial.printf("Failed to open file %s\n", filename);
    return false;
  }

/*
  size_t i = 0;
  while (file.available())
  {
    buffer[i++] = file.read();
  }
  buffer[i] = '\0';
*/

  size_t fileSize = file.size();
  Serial.printf("File size: %d bytes\n", fileSize);
  if (fileSize > maxSize)
  {
    Serial.printf("File %s is too big\n", filename);
    file.close();
    return false;
  }
  
  file.readBytes(buffer, fileSize);
  buffer[fileSize] = '\0'; // Null-terminate the text
  file.close();
  return true;
}

  // ===========================================================================================================================
  // ===========================================================================================================================


void SD_TEST_listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        SD_TEST_listDir(fs, file.path(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void SD_TEST_createDir(fs::FS &fs, const char *path)
{
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path))
  {
    Serial.println("Dir created");
  }
  else
  {
    Serial.println("mkdir failed");
  }
}

void SD_TEST_removeDir(fs::FS &fs, const char *path)
{
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path))
  {
    Serial.println("Dir removed");
  }
  else
  {
    Serial.println("rmdir failed");
  }
}

void SD_TEST_readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}

void SD_TEST_writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void SD_TEST_appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void SD_TEST_renameFile(fs::FS &fs, const char *path1, const char *path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
  }
  else
  {
    Serial.println("Rename failed");
  }
}

void SD_TEST_deleteFile(fs::FS &fs, const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

void SD_TEST_testFileIO(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  static uint8_t buf[SPI_DMA_MAX];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file)
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len)
    {
      size_t toRead = len;
      if (toRead > SPI_DMA_MAX)
      {
        toRead = SPI_DMA_MAX;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read in %lu ms\n", flen, end);
    file.close();
  }
  else
  {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 512; i++)
  {
    file.write(buf, SPI_DMA_MAX);
  }
  end = millis() - start;
  Serial.printf("%u bytes written in %lu ms\n", 2048 * SPI_DMA_MAX, end);
  file.close();
}

void SD_TEST()
{
#if defined(USE_SD_CARD_SPI)||defined(USE_SD_CARD_MMC)  
  SD_TEST_listDir(FILESYS, "/", 0);
  SD_TEST_createDir(FILESYS, "/mydir");
  SD_TEST_listDir(FILESYS, "/", 0);
  SD_TEST_removeDir(FILESYS, "/mydir");
  SD_TEST_listDir(FILESYS, "/", 2);
  SD_TEST_writeFile(FILESYS, "/hello.txt", "Hello ");
  SD_TEST_appendFile(FILESYS, "/hello.txt", "World!\n");
  SD_TEST_readFile(FILESYS, "/hello.txt");
  SD_TEST_deleteFile(FILESYS, "/foo.txt");
  SD_TEST_renameFile(FILESYS, "/hello.txt", "/foo.txt");
  SD_TEST_readFile(FILESYS, "/foo.txt");
  SD_TEST_testFileIO(FILESYS, "/test.txt");
#endif
}
