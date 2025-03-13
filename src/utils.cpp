
#include "Arduino.h"
#include "utils.h"

const char* DAYS3[] = { "PON", "TOR", "SRE", "CET", "PET", "SOB", "NED" };
const char* DAYSF[] = { "ponedeljek", "torek", "sreda", "cetrtek", "petek", "sobota", "nedelja" };

// remove non-printable chars
void TrimNonPrintable (String& Str) {
  Str.trim(); // remove leading and trailing spaces
  for (int16_t i = 0; i < Str.length(); i++)
  {
    char c = Str.charAt(i);
    if ((c < 32) || (c > 126)) {
      Str.remove(i, 1);
      i--;
      }
  }  
}

void TrimNumDot (String& Str) {
  Str.trim(); // remove leading and trailing spaces
  TrimNonPrintable(Str);
  for (int16_t i = 0; i < Str.length(); i++)
  {
    char c = Str.charAt(i);
    if (((c < '0') || (c > '9')) && (c != '.')) {
      Str.remove(i, 1);
      i--;
      }
  }  
}

void TrimAlfaNum (String& Str) {
  bool validChar;
  Str.trim(); // remove leading and trailing spaces
  TrimNonPrintable(Str);
  for (uint16_t i = 0; i < Str.length(); i++)
  {
    char c = Str.charAt(i);
    validChar = ((c >= '0') && (c <= '9')) ||
                ((c >= 'A') && (c <= 'Z')) ||
                ((c >= 'a') && (c <= 'z'));
    if (!validChar) {
      Str.remove(i, 1);
      i--;
      }
  }  
}

// remove double chars
void TrimDoubleChars (String& Str, char cc) {
  if (Str.length() < 2) return;
  bool Found;
  Str.trim(); // remove leading and trailing spaces
  char c1, c2;
  unsigned int i = 0;
  unsigned int len = Str.length()-1;
  while (i < len) {
    c1 = Str.charAt(i);
    c2 = Str.charAt(i+1);
    Found = ((c1 == cc) && (c2 == cc));
    i++;
    if (Found) {
      Str.remove(i, 1);
      i--;
      len--;
      }
  }  
}

void TrimDoubleSpaces (String& Str) {
  if (Str.length() < 2) return;
  char c1, c2;
  unsigned int i = 0;
  unsigned int len = Str.length()-1;
  while (i < len) {
    c1 = Str.charAt(i);
    c2 = Str.charAt(i+1);
    if ((c1 == c2) && (c2 == SPACE)) {
      Str.remove(i, 1);
      len--;
    } else {
      i++;
    }
  }  
}

bool IsUppercaseChar(char chr) {
  return ((chr >= 65) && (chr <= 90));  
}

int FindUppercaseChar(String &Str, const int StartAt = 0) {
  char chr;
  for (unsigned int i = StartAt; i < Str.length(); i++)
  {
    chr = Str.charAt(i);
    if ((chr >= 65) && (chr <= 90)) // A..Z
    {
      return i;
    }    
  }
  return -1;  
}

String FindJsonParam(String& inStr, String needParam, int& Position)
{
    int indexStart = inStr.indexOf(needParam, Position);
    Position = -1; // status "not found" until found
    if (indexStart > 0) {
        int indexStop = inStr.indexOf(",", indexStart);
        Position = indexStop;
        if (indexStop <= indexStart) {
          Position = -1; // status "not found"
          return "/";
        }
        int CountChar = needParam.length();
        return inStr.substring(indexStart+CountChar+2, indexStop);
    }
    return "/";
}


String FindXMLParam(String& inStr, String needParam, int& Position)
{
    String searchString = "<"+needParam+">";
    int indexStart = inStr.indexOf(searchString, Position);
    Position = -1; // status "not found" until found
    if (indexStart > 0) {
        int indexStop = inStr.indexOf("</"+needParam+">", indexStart);  
        Position = indexStop;
        if (indexStop <= indexStart) {
          Position = -1; // status "not found"
          return "/";
        }
        int CountChar = needParam.length();
        return inStr.substring(indexStart+CountChar+2, indexStop);
    }
    return "/";
}

//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// UTF8-Decoder: convert UTF8-string to extended ASCII.                                            *
// Convert a single Character from UTF8 to Extended ASCII.                                         *
// Return "0" if a byte has to be ignored.                                                         *
//**************************************************************************************************
char utf8ascii ( char ascii )
{
  static const char lut_C3[] = { "AAAAAAACEEEEIIIIDNOOOOO#0UUUU###"
                                 "aaaaaaaceeeeiiiidnooooo##uuuuyyy" } ; 
  static const char lut_C4[] = { "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGg"
                                 "GgGgHhHhIiIiIiIiIiJjJjKkkLlLlLlL" } ;
  static const char lut_C5[] = { "lLlNnNnNnnnnOoOoOoOoRrRrRrSsSsSs"
                                 "SsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs" } ;

  static char       c1 ;              // Last character buffer
  char              res = '\0' ;      // Result, default 0

  if ( ascii <= 0x7F )                // Standard ASCII-set 0..0x7F handling
  {
    c1 = 0 ;
    res = ascii ;                     // Return unmodified
  }
  else
  {
    switch ( c1 )                     // Conversion depending on first UTF8-character
    {
      case 0xC2: res = '~' ;
        break ;
      case 0xC3: res = lut_C3[ascii - 128] ;
        break ;
      case 0xC4: res = lut_C4[ascii - 128] ;
        break ;
      case 0xC5: res = lut_C5[ascii - 128] ;
        break ;
      case 0x82: if ( ascii == 0xAC )
        {
          res = 'E' ;                 // Special case Euro-symbol
        }
    }
    c1 = ascii ;                      // Remember actual character
  }
  return res ;                        // Otherwise: return zero, if character has to be ignored
}


//**************************************************************************************************
//                                U T F 8 A S C I I _ I P                                          *
//**************************************************************************************************
// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!).                          *
//**************************************************************************************************
void utf8ascii_ip ( char* s )
{
  int  i, k = 0 ;                     // Indexes for in en out string
  char c ;

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      s[k++] = c ;                    // Yes, put in output string
    }
  }
  s[k] = 0 ;                          // Take care of delimeter
}


//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// Conversion UTF8-String to Extended ASCII String.                                                *
//**************************************************************************************************
String utf8ascii ( const char* s )
{
  int  i ;                            // Index for input string
  char c ;
  String res = "" ;                   // Result string

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      res += String ( c ) ;           // Yes, put in output string
    }
  }
  return res ;
}

//==========================================================================================================
//==========================================================================================================
//==========================================================================================================

// 8-bit CRC calculation with 0x97 polynome

//Lookup table for polynome = 0x97
static const uint8_t ab_CRC8_LUT[256] = {
  0x00, 0x97, 0xB9, 0x2E, 0xE5, 0x72, 0x5C, 0xCB, 0x5D, 0xCA, 0xE4, 0x73, 0xB8, 0x2F, 0x01, 0x96,
  0xBA, 0x2D, 0x03, 0x94, 0x5F, 0xC8, 0xE6, 0x71, 0xE7, 0x70, 0x5E, 0xC9, 0x02, 0x95, 0xBB, 0x2C,
  0xE3, 0x74, 0x5A, 0xCD, 0x06, 0x91, 0xBF, 0x28, 0xBE, 0x29, 0x07, 0x90, 0x5B, 0xCC, 0xE2, 0x75,
  0x59, 0xCE, 0xE0, 0x77, 0xBC, 0x2B, 0x05, 0x92, 0x04, 0x93, 0xBD, 0x2A, 0xE1, 0x76, 0x58, 0xCF,
  0x51, 0xC6, 0xE8, 0x7F, 0xB4, 0x23, 0x0D, 0x9A, 0x0C, 0x9B, 0xB5, 0x22, 0xE9, 0x7E, 0x50, 0xC7,
  0xEB, 0x7C, 0x52, 0xC5, 0x0E, 0x99, 0xB7, 0x20, 0xB6, 0x21, 0x0F, 0x98, 0x53, 0xC4, 0xEA, 0x7D,
  0xB2, 0x25, 0x0B, 0x9C, 0x57, 0xC0, 0xEE, 0x79, 0xEF, 0x78, 0x56, 0xC1, 0x0A, 0x9D, 0xB3, 0x24,
  0x08, 0x9F, 0xB1, 0x26, 0xED, 0x7A, 0x54, 0xC3, 0x55, 0xC2, 0xEC, 0x7B, 0xB0, 0x27, 0x09, 0x9E,
  0xA2, 0x35, 0x1B, 0x8C, 0x47, 0xD0, 0xFE, 0x69, 0xFF, 0x68, 0x46, 0xD1, 0x1A, 0x8D, 0xA3, 0x34,
  0x18, 0x8F, 0xA1, 0x36, 0xFD, 0x6A, 0x44, 0xD3, 0x45, 0xD2, 0xFC, 0x6B, 0xA0, 0x37, 0x19, 0x8E,
  0x41, 0xD6, 0xF8, 0x6F, 0xA4, 0x33, 0x1D, 0x8A, 0x1C, 0x8B, 0xA5, 0x32, 0xF9, 0x6E, 0x40, 0xD7,
  0xFB, 0x6C, 0x42, 0xD5, 0x1E, 0x89, 0xA7, 0x30, 0xA6, 0x31, 0x1F, 0x88, 0x43, 0xD4, 0xFA, 0x6D,
  0xF3, 0x64, 0x4A, 0xDD, 0x16, 0x81, 0xAF, 0x38, 0xAE, 0x39, 0x17, 0x80, 0x4B, 0xDC, 0xF2, 0x65,
  0x49, 0xDE, 0xF0, 0x67, 0xAC, 0x3B, 0x15, 0x82, 0x14, 0x83, 0xAD, 0x3A, 0xF1, 0x66, 0x48, 0xDF,
  0x10, 0x87, 0xA9, 0x3E, 0xF5, 0x62, 0x4C, 0xDB, 0x4D, 0xDA, 0xF4, 0x63, 0xA8, 0x3F, 0x11, 0x86,
  0xAA, 0x3D, 0x13, 0x84, 0x4F, 0xD8, 0xF6, 0x61, 0xF7, 0x60, 0x4E, 0xD9, 0x12, 0x85, 0xAB, 0x3C};



  /* CRC 0x97 Polynomial, 64-bit input data, right alignment, calculation over 64 bits */
  uint8_t CRC_SPI_97_64bit (uint64_t dw_InputData)
{
uint8_t b_Index = 0;
uint8_t b_CRC = 0;
  b_Index = (uint8_t)((dw_InputData >> 56u) & (uint64_t)0x000000FFu);
  b_CRC = (uint8_t)((dw_InputData >> 48u) & (uint64_t)0x000000FFu);
  b_Index = b_CRC ^ ab_CRC8_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 40u) & (uint64_t)0x000000FFu);
  b_Index = b_CRC ^ ab_CRC8_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 32u) & (uint64_t)0x000000FFu);
  b_Index = b_CRC ^ ab_CRC8_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 24u) & (uint64_t)0x000000FFu);
  b_Index = b_CRC ^ ab_CRC8_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 16u) & (uint64_t)0x000000FFu);
  b_Index = b_CRC ^ ab_CRC8_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 8u) & (uint64_t)0x000000FFu);
  b_Index = b_CRC ^ ab_CRC8_LUT[b_Index];
  b_CRC = (uint8_t)(dw_InputData & (uint64_t)0x000000FFu);
  b_Index = b_CRC ^ ab_CRC8_LUT[b_Index];
  b_CRC = ab_CRC8_LUT[b_Index];
  return b_CRC;
}

//==========================================================================================================
//==========================================================================================================
//==========================================================================================================

// 6-bit CRC calculation with 0x43 polynome for BiSS

uint8_t ab_CRC6_LUT[64] = {
  0x00, 0x03, 0x06, 0x05, 0x0C, 0x0F, 0x0A, 0x09,
  0x18, 0x1B, 0x1E, 0x1D, 0x14, 0x17, 0x12, 0x11,
  0x30, 0x33, 0x36, 0x35, 0x3C, 0x3F, 0x3A, 0x39,
  0x28, 0x2B, 0x2E, 0x2D, 0x24, 0x27, 0x22, 0x21,
  0x23, 0x20, 0x25, 0x26, 0x2F, 0x2C, 0x29, 0x2A,
  0x3B, 0x38, 0x3D, 0x3E, 0x37, 0x34, 0x31, 0x32,
  0x13, 0x10, 0x15, 0x16, 0x1F, 0x1C, 0x19, 0x1A,
  0x0B, 0x08, 0x0D, 0x0E, 0x07, 0x04, 0x01, 0x02};

/*32-bit input data, right alignment, Calculation over 24 bits (mult. of 6) */
uint8_t CRC_BiSS_43_24bit (uint32_t w_InputData)
{
  uint8_t b_Index = 0;
  uint8_t b_CRC = 0;
  b_Index = (uint8_t )(((uint32_t)w_InputData >> 18u) & 0x0000003Fu);
  b_CRC = (uint8_t )(((uint32_t)w_InputData >> 12u) & 0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = (uint8_t )(((uint32_t)w_InputData >> 6u) & 0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = (uint8_t )((uint32_t)w_InputData & 0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = ab_CRC6_LUT[b_Index];
  return b_CRC;
}

/*64-bit input data, right alignment, Calculation over 42 bits (mult. of 6) */
uint8_t CRC_BiSS_43_42bit (uint64_t dw_InputData)
{
  uint8_t b_Index = 0;
  uint8_t b_CRC = 0;
  b_Index = (uint8_t)((dw_InputData >> 36u) & (uint64_t)0x00000003Fu);
  b_CRC = (uint8_t)((dw_InputData >> 30u) & (uint64_t)0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 24u) & (uint64_t)0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 18u) & (uint64_t)0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 12u) & (uint64_t)0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = (uint8_t)((dw_InputData >> 6u) & (uint64_t)0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = (uint8_t)(dw_InputData & (uint64_t)0x0000003Fu);
  b_Index = b_CRC ^ ab_CRC6_LUT[b_Index];
  b_CRC = ab_CRC6_LUT[b_Index];
  return b_CRC;
}

/*
void test() {
uint8_t rx_buffer[6]; // contains received bytes

// TODO: load rx_buffer array with received data from the encodre
uint64_t dw_CRCinputData = 0;
uint8_t calculated_crc=0;
dw_CRCinputData = ((uint64_t)rx_buffer[0] << 32) + ((uint64_t)rx_buffer[1] << 24) +
((uint64_t)rx_buffer[2] << 16) + ((uint64_t)rx_buffer[3] << 8) +
((uint64_t)rx_buffer[4] << 0 );
calculated_crc = ~(CRC_SPI_97_64bit(dw_CRCinputData))& 0xFF; //inverted CRC
}
*/

