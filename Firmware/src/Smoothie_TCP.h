#ifndef SMOOTCPCLIENT_H
#define SMOOTCPCLIENT_H

bool Smoothie_TCPclientConnect(void);
bool Smoothie_TCPclientRequest(const char Text[]);
void Smoothie_TCPclientDisconnect(void);

extern String Smoothie_TCPresponse;

#endif // SMOOTCPCLIENT_H