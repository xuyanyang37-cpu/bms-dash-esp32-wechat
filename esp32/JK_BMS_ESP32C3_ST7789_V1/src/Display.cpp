#include "Display.h"
void Display::begin(){
  pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,HIGH);tft_.init();tft_.setRotation(1);tft_.fillScreen(TFT_BLACK);
  sprite_.setColorDepth(16);sprite_.createSprite(320,170);sprite_.fillSprite(TFT_BLACK);sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
  sprite_.drawString("JK BMS BLE",8,8,4);sprite_.drawString("Waiting...",8,50,2);sprite_.pushSprite(0,0);
}
void Display::update(const BmsData&d){
  sprite_.fillSprite(TFT_BLACK);
  sprite_.setTextColor(d.online?TFT_GREEN:TFT_RED,TFT_BLACK);sprite_.drawString(d.online?"BLE OK":"BLE OFF",5,3,2);
  sprite_.setTextColor(TFT_WHITE,TFT_BLACK);sprite_.drawString(d.deviceName.length()?d.deviceName:"JK-BMS",75,3,2);
  sprite_.setTextColor(TFT_CYAN,TFT_BLACK);sprite_.drawString(String(d.totalVoltage,1)+" V",5,25,6);
  sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);sprite_.drawString(String(d.current,1)+" A",5,75,4);
  sprite_.setTextColor(TFT_ORANGE,TFT_BLACK);sprite_.drawString(String(d.power,0)+" W",155,75,4);
  sprite_.setTextColor(TFT_GREEN,TFT_BLACK);sprite_.drawString("Δ"+String(d.deltaCellVoltage*1000,0)+"mV",5,112,4);
  sprite_.setTextColor(TFT_WHITE,TFT_BLACK);sprite_.drawString("MIN "+String(d.minCellVoltage,3),155,115,2);
  sprite_.drawString("MAX "+String(d.maxCellVoltage,3),155,137,2);
  sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);sprite_.drawString("MOS "+String(d.mosTemperature,1)+"C",5,145,2);
  sprite_.drawString("CELL "+String(d.cellCount),95,145,2);
  sprite_.pushSprite(0,0);
}
