#ifndef HPTCPCLIENT_H
#define HPTCPCLIENT_H

bool HP_TCPclientConnect(void);
bool HP_TCPclientRequest(const char Text[]);
void HP_TCPclientDisconnect(void);

extern String HP_TCPresponse;

#endif // HPTCPCLIENT_H