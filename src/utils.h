#ifndef UTILS_H
#define UTILS_H

#define TAB  9
#define SPACE 32

extern const char* DAYS3[];
extern const char* DAYSF[];

void TrimNonPrintable (String& Str);
void TrimNumDot (String& Str);
void TrimAlfaNum (String& Str);
void TrimDoubleChars (String& Str, char cc);
void TrimDoubleSpaces (String& Str);

bool IsUppercaseChar(char chr);
int FindUppercaseChar(String &Str, const int StartAt);

String FindJsonParam(String& inStr, String needParam, int& Position);

String FindXMLParam(String& inStr, String needParam, int& Position);

char        utf8ascii ( char ascii ) ;                             // Convert UTF to Ascii
void        utf8ascii_ip ( char* s ) ;                             // Convert UTF to Ascii in place
String      utf8ascii ( const char* s ) ;                          // Convert UTF to Ascii as String

#endif
