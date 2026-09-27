#include "Display.h"

static uint16_t lerp565(uint16_t a,uint16_t b,uint16_t t){
  uint8_t ar=(a>>11)&0x1F, ag=(a>>5)&0x3F, ab=a&0x1F;
  uint8_t br=(b>>11)&0x1F, bg=(b>>5)&0x3F, bb=b&0x1F;
  uint8_t rr=ar+((br-ar)*t)/100, rg=ag+((gg-bg)*t)/100, rb=ab+((bb-ab)*t)/100;
  return (rr<<11)|(rg<<5)|rb;
}

void Display::begin(){
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,HIGH);
  tft_.init();
  tft_.setRotation(1);
  tft_.fillScreen(TFT_BLACK);
  sprite_.setColorDepth(16);
  sprite_.createSprite(320,170);
  sprite_.fillSprite(TFT_BLACK);
  sprite_.pushSprite(0,0);
}

void Display::update(const BmsData& d){
  sprite_.fillSprite(TFT_BLACK);

  if(d.bootState==BOOT_SCANNING || d.bootState==BOOT_START){
    sprite_.setTextColor(TFT_CYAN,TFT_BLACK);
    sprite_.drawCentreString("连接电池",160,10,4);
    sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
    sprite_.drawCentreString(d.statusMessage.length()?d.statusMessage:"扫描蓝牙...",160,48,2);
    int w=250;
    sprite_.drawRoundRect(35,78,w,18,5,TFT_DARKGREY);
    int progress=(d.scanAttempt*100)/3;
    if(progress>100) progress=100;
    sprite_.fillRoundRect(38,81,(w-6)*progress/100,12,4,TFT_BLUE);
    sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
    sprite_.drawCentreString(String(d.scanAttempt)+"/3",160,105,4);
    sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
    sprite_.drawCentreString("自动扫描并连接JK保护板",160,145,2);
    sprite_.pushSprite(0,0);
    return;
  }

  if(d.bootState==BOOT_CONNECTING){
    sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
    sprite_.drawCentreString("正在连接",160,20,4);
    sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
    sprite_.drawCentreString(d.mac.length()?d.mac:"JK-BMS",160,68,2);
    sprite_.drawCentreString("正在建立蓝牙连接...",160,100,2);
    sprite_.pushSprite(0,0);
    return;
  }

  if((d.bootState==BOOT_HOTSPOT || d.hotspot) && !d.online){
    sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
    sprite_.drawCentreString("热点设置",160,8,4);
    sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
    sprite_.drawCentreString("WiFi: JK-BMS-SETUP",160,48,2);
    sprite_.setTextColor(TFT_CYAN,TFT_BLACK);
    sprite_.drawCentreString("手机连接后打开网页",160,75,2);
    sprite_.drawCentreString(d.hotspotIp.length()?d.hotspotIp:"192.168.4.1",160,105,4);
    sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
    sprite_.drawCentreString("扫描 / 选择 / 连接电池",160,140,2);
    sprite_.pushSprite(0,0);
    return;
  }

  sprite_.setTextColor(TFT_GREEN,TFT_BLACK);
  sprite_.drawCentreString(String(d.soc,0)+"%",78,30,7);
  sprite_.setTextColor(TFT_WHITE,TFT_BLACK);
  sprite_.drawCentreString("剩余电量",78,83,2);

  sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
  sprite_.drawString(d.online?"蓝牙已连接":"蓝牙断开",5,5,2);
  if(d.deviceName.length()) sprite_.drawRightString(d.deviceName,315,5,2);

  sprite_.setTextColor(TFT_CYAN,TFT_BLACK);
  sprite_.drawString("电压",145,29,2);
  sprite_.drawRightString(String(d.totalVoltage,1)+" V",315,29,4);

  sprite_.setTextColor(TFT_YELLOW,TFT_BLACK);
  sprite_.drawString("电流",145,58,2);
  sprite_.drawRightString(String(d.current,1)+" A",315,58,4);

  sprite_.setTextColor(TFT_ORANGE,TFT_BLACK);
  sprite_.drawString("功率",145,87,2);
  sprite_.drawRightString(String(d.power,0)+" W",315,87,4);

  sprite_.setTextColor(TFT_CYAN,TFT_BLACK);
  sprite_.drawString("剩余容量",145,116,2);
  sprite_.drawRightString(String(d.remainingCapacityAh,1)+" Ah",315,116,4);

  sprite_.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
  sprite_.drawString("剩余里程",145,145,2);
  sprite_.drawRightString(String(d.remainingRangeKm,1)+" km",315,145,2);

  // 底部显示剩余里程/剩余电量的进度条。
  // 这里使用 SOC 作为最稳定的电量百分比基准。
  float ratio=d.soc/100.0f;
  if(ratio<0) ratio=0;
  if(ratio>1) ratio=1;

  const int x=8,y=162,w=304,h=6;
  for(int i=0;i<16;i++){
    float p0=i/16.0f;
    uint16_t color;
    if(p0<0.25f) color=lerp565(TFT_RED,TFT_ORANGE,(uint16_t)(p0*400));
    else if(p0<0.5f) color=lerp565(TFT_ORANGE,TFT_YELLOW,(uint16_t)((p0-0.25f)*400));
    else if(p0<0.75f) color=lerp565(TFT_YELLOW,TFT_GREEN,(uint16_t)((p0-0.5f)*400));
    else color=lerp565(TFT_GREEN,TFT_CYAN,(uint16_t)((p0-0.75f)*400));
    int sx=x+(w*i)/16;
    int sw=(w*(i+1))/16-(w*i)/16;
    if((i+1)/16.0f<=ratio) sprite_.fillRect(sx,y,sw,h,color);
    else sprite_.drawRect(sx,y,sw,h,TFT_DARKGREY);
  }

  sprite_.pushSprite(0,0);
}
