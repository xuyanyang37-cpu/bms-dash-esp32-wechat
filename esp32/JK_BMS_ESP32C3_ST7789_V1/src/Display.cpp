#include "Display.h"

void Display::begin(){
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,HIGH);
  tft_.init();
  tft_.setRotation(1);
  tft_.fillScreen(TFT_BLACK);

  sprite_.setColorDepth(16);
  sprite_.createSprite(320,170);
  sprite_.fillSprite(TFT_BLACK);
  sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
  sprite_.drawString("JK BMS",110,20,4);
  sprite_.drawString("Starting...",100,65,2);
  sprite_.pushSprite(0,0);
}

void Display::update(const BmsData& d){
  sprite_.fillSprite(TFT_BLACK);

  if(d.bootState==BOOT_SCANNING || d.bootState==BOOT_START){
    sprite_.setTextColor(TFT_CYAN,TFT_BLACK);
    sprite_.drawString("CONNECT BATTERY",72,8,2);
    sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
    sprite_.drawString(d.statusMessage.length()?d.statusMessage:"Scanning BLE...",45,38,2);

    int w=250;
    sprite_.drawRect(35,72,w,18,TFT_DARKGREY);
    int progress=(d.scanAttempt*100)/3;
    if(progress>100) progress=100;
    sprite_.fillRect(38,75,(w-6)*progress/100,12,TFT_BLUE);

    sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
    sprite_.drawCentreString(String(d.scanAttempt)+"/3",160,96,4);
    sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
    sprite_.drawCentreString("自动扫描并连接JK保护板",160,132,2);
    sprite_.pushSprite(0,0);
    return;
  }

  if(d.bootState==BOOT_CONNECTING){
    sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
    sprite_.drawCentreString("CONNECTING",160,20,4);
    sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
    sprite_.drawCentreString(d.mac.length()?d.mac:"JK-BMS",160,65,2);
    sprite_.drawCentreString("正在建立蓝牙连接...",160,95,2);
    sprite_.pushSprite(0,0);
    return;
  }

  if((d.bootState==BOOT_HOTSPOT || d.hotspot) && !d.online){
    sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
    sprite_.drawCentreString("HOTSPOT MODE",160,8,4);
    sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
    sprite_.drawCentreString("WiFi: JK-BMS-SETUP",160,48,2);
    sprite_.setTextColor(TFT_CYAN,TFT_BLACK);
    sprite_.drawCentreString("网页设置蓝牙参数",160,75,2);
    sprite_.drawCentreString(d.hotspotIp.length()?d.hotspotIp:"192.168.4.1",160,105,4);
    sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
    sprite_.drawCentreString("浏览器打开上面的IP",160,140,2);
    sprite_.pushSprite(0,0);
    return;
  }

  sprite_.setTextColor(d.online?TFT_GREEN:TFT_RED,TFT_BLACK);
  sprite_.drawString(d.online?(d.hotspot?"BLE OK AP":"BLE OK"):"BLE OFF",5,3,2);
  sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
  sprite_.drawString(d.deviceName.length()?d.deviceName:"JK-BMS",75,3,2);

  sprite_.setTextColor(TFT_CYAN,TFT_BLACK);
  sprite_.drawString(String(d.totalVoltage,1)+" V",5,25,6);

  sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
  sprite_.drawString(String(d.current,1)+" A",5,75,4);

  sprite_.setTextColor(TFT_ORANGE,TFT_BLACK);
  sprite_.drawString(String(d.power,0)+" W",155,75,4);

  sprite_.setTextColor(TFT_GREEN,TFT_BLACK);
  sprite_.drawString("SOC "+String(d.soc,0)+"%",5,112,4);

  sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
  sprite_.drawString("dV "+String(d.deltaCellVoltage*1000,0)+"mV",155,115,2);
  sprite_.drawString("MAX "+String(d.maxCellVoltage,3),155,137,2);

  sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
  sprite_.drawString("MOS "+String(d.mosTemperature,1)+"C",5,145,2);
  sprite_.drawString("CELL "+String(d.cellCount),95,145,2);

  sprite_.pushSprite(0,0);
}
