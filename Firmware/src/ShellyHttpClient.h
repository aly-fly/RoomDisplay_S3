
#ifndef __SHELLYHTTPCLIENT_H_
#define __SHELLYHTTPCLIENT_H_

bool ShellyGetPower(String URL, float &power);
bool ShellyGetTemperature(void);
bool ShellyGetSwitch1(void);
bool ShellyGetSwitch2(void);

extern float ShellyTotalPowerHP, ShellyTotalPowerAll;
extern String sShellyTemperature;
extern bool Shelly1ON, Shelly2ON;
extern float Shelly2Power;

#endif // __SHELLYHTTPCLIENT_H_