#ifndef __ARSO_XML_H_
#define __ARSO_XML_H_

bool GetARSOdata(void);
bool GetARSOmeteogram(void);

void InvalidateArsoData(void);

struct ArsoWeather_t
{
    int8_t DayN;
    String DayName;
    String PartOfDay;
    String Sky;
    String Rain;
    String WeatherIcon;
    String WindIcon;
    String Temperature;
    float TemperatureN;
    float RainN;
    float SnowN;
    float WindN;
};

extern ArsoWeather_t ArsoWeather[4];
extern ArsoWeather_t ArsoMeteogram[MTG_NUMPTS];
extern String SunRiseTime, SunSetTime;

#endif // __ARSO_XML_H_