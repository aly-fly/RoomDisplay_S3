#ifndef MYWIFI_H
#define MYWIFI_H

#include "__CONFIG.h"
#include "___CONFIG_SECRETS.h"


//enum WifiState_t {disconnected, connected};
void WifiInit(void);
void WifiReconnectIfNeeded(void);
void WifiDisconnect(void);

//extern WifiState_t WifiState;
extern bool inHomeLAN;

#endif // MYWIFI_H
