#ifndef DISPLAY_H
#define DISPLAY_H
#include <TFT_eSPI.h>
#include "BmsData.h"
class Display{
public:void begin();void update(const BmsData&d);
private:TFT_eSPI tft_;TFT_eSprite sprite_{&tft_};
};
#endif
