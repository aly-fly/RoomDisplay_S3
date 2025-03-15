
#ifndef __GLOBALVARIABLES_H_
#define __GLOBALVARIABLES_H_

#include <stdint.h>

// create static buffer for reading stream from the server
extern uint8_t gBuffer[3000];
extern uint8_t GIFimage[100000];

void globalVariablesInit(void);
void globalVariablesFree(void);

#endif //__GLOBALVARIABLES_H_
