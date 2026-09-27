#include "Display.h"
#include "FontGB2312.h"

static uint16_t lerp565(uint16_t a,uint16_t b,uint16_t t){
  uint8_t ar=(a>>11)&0x1F, ag=(a>>5)&0x3F, ab=a&0x1F;
  uint8_t br=(b>>11)&0x1F, bg=(b>>5)&0x3F, bb=b&0x1F;
  uint8_t rr=ar+((br-ar)*t)/100;
  uint8_t rg=ag+((bg-ag)*t)/100;
  uint8_t rb=ab+((bb-ab)*t)/100;
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

  // ============================================================
  // 启动 / 自动扫描页面
  // ============================================================
  if(d.bootState==BOOT_SCANNING || d.bootState==BOOT_START){
    FontGB2312::drawCenterString(
      sprite_,160,7,"连接电池",TFT_CYAN,TFT_BLACK,2);

    FontGB2312::drawCenterString(
      sprite_,160,43,
      d.statusMessage.length()?d.statusMessage:"扫描蓝牙...",
      TFT_WHITE,TFT_BLACK,1);

    int w=250;
    sprite_.drawRoundRect(35,72,w,18,5,TFT_DARKGREY);

    int progress=(d.scanAttempt*100)/3;
    if(progress>100) progress=100;

    if(progress>0){
      sprite_.fillRoundRect(
        38,75,(w-6)*progress/100,12,4,TFT_BLUE);
    }

    FontGB2312::drawCenterString(
      sprite_,160,96,
      String(d.scanAttempt)+"/3",
      TFT_YELLOW,TFT_BLACK,1);

    FontGB2312::drawCenterString(
      sprite_,160,135,
      "自动扫描并连接JK保护板",
      TFT_LIGHTGREY,TFT_BLACK,1);

    sprite_.pushSprite(0,0);
    return;
  }

  // ============================================================
  // 正在连接
  // ============================================================
  if(d.bootState==BOOT_CONNECTING){
    FontGB2312::drawCenterString(
      sprite_,160,8,"正在连接",TFT_YELLOW,TFT_BLACK,2);

    FontGB2312::drawCenterString(
      sprite_,160,50,
      d.mac.length()?d.mac:"JK-BMS",
      TFT_WHITE,TFT_BLACK,1);

    FontGB2312::drawCenterString(
      sprite_,160,82,
      "正在建立蓝牙连接...",
      TFT_WHITE,TFT_BLACK,1);

    sprite_.pushSprite(0,0);
    return;
  }

  // ============================================================
  // 热点设置页面
  // ============================================================
  if((d.bootState==BOOT_HOTSPOT || d.hotspot) && !d.online){
    FontGB2312::drawCenterString(
      sprite_,160,5,"热点设置",TFT_YELLOW,TFT_BLACK,2);

    FontGB2312::drawCenterString(
      sprite_,160,43,"WiFi: JK-BMS-SETUP",
      TFT_WHITE,TFT_BLACK,1);

    FontGB2312::drawCenterString(
      sprite_,160,70,"手机连接后打开网页",
      TFT_CYAN,TFT_BLACK,1);

    FontGB2312::drawCenterString(
      sprite_,160,94,
      d.hotspotIp.length()?d.hotspotIp:"192.168.4.1",
      TFT_CYAN,TFT_BLACK,1);

    FontGB2312::drawCenterString(
      sprite_,160,130,"扫描 / 选择 / 连接电池",
      TFT_LIGHTGREY,TFT_BLACK,1);

    sprite_.pushSprite(0,0);
    return;
  }

  // ============================================================
  // 主 BMS 数据页面
  // ============================================================

  // 左侧 SOC 数值仍然使用 TFT_eSPI 大号数字字体。
  sprite_.setTextColor(TFT_GREEN,TFT_BLACK);
  sprite_.drawCentreString(String(d.soc,0)+"%",78,25,7);

  // 左侧中文标签。
  FontGB2312::drawCenterString(
    sprite_,78,80,"剩余电量",TFT_WHITE,TFT_BLACK,1);

  // 顶部连接状态。
  FontGB2312::drawText(
    sprite_,5,3,
    d.online?"蓝牙已连接":"蓝牙断开",
    TFT_LIGHTGREY,TFT_BLACK,1);

  if(d.deviceName.length()){
    FontGB2312::drawRightString(
      sprite_,315,3,d.deviceName,
      TFT_LIGHTGREY,TFT_BLACK,1);
  }

  // 电压。
  FontGB2312::drawText(
    sprite_,145,27,"电压",
    TFT_CYAN,TFT_BLACK,1);
  sprite_.drawRightString(
    String(d.totalVoltage,1)+" V",315,25,4);

  // 电流。
  FontGB2312::drawText(
    sprite_,145,56,"电流",
    TFT_YELLOW,TFT_BLACK,1);
  sprite_.drawRightString(
    String(d.current,1)+" A",315,54,4);

  // 功率。
  FontGB2312::drawText(
    sprite_,145,85,"功率",
    TFT_ORANGE,TFT_BLACK,1);
  sprite_.drawRightString(
    String(d.power,0)+" W",315,83,4);

  // 剩余容量。
  FontGB2312::drawText(
    sprite_,145,114,"剩余容量",
    TFT_CYAN,TFT_BLACK,1);
  sprite_.drawRightString(
    String(d.remainingCapacityAh,1)+" Ah",315,112,4);

  // 剩余里程。
  FontGB2312::drawText(
    sprite_,145,143,"剩余里程",
    TFT_LIGHTGREY,TFT_BLACK,1);
  sprite_.drawRightString(
    String(d.remainingRangeKm,1)+" km",315,141,2);

  // ============================================================
  // 底部 SOC 渐变进度条
  // ============================================================
  float ratio=d.soc/100.0f;
  if(ratio<0) ratio=0;
  if(ratio>1) ratio=1;

  const int x=8,y=162,w=304,h=6;

  for(int i=0;i<16;i++){
    float p0=i/16.0f;
    uint16_t color;

    if(p0<0.25f){
      color=lerp565(
        TFT_RED,TFT_ORANGE,
        (uint16_t)(p0*400));
    }else if(p0<0.5f){
      color=lerp565(
        TFT_ORANGE,TFT_YELLOW,
        (uint16_t)((p0-0.25f)*400));
    }else if(p0<0.75f){
      color=lerp565(
        TFT_YELLOW,TFT_GREEN,
        (uint16_t)((p0-0.5f)*400));
    }else{
      color=lerp565(
        TFT_GREEN,TFT_CYAN,
        (uint16_t)((p0-0.75f)*400));
    }

    int sx=x+(w*i)/16;
    int sw=(w*(i+1))/16-(w*i)/16;

    if((i+1)/16.0f<=ratio){
      sprite_.fillRect(sx,y,sw,h,color);
    }else{
      sprite_.drawRect(sx,y,sw,h,TFT_DARKGREY);
    }
  }

  sprite_.pushSprite(0,0);
}
