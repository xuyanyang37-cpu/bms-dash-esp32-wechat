#include "Display.h"
#include "FontGB2312.h"
#include <math.h>

/*
 * JK BMS ST7789 1.9" / 320x170
 * UI 2.0
 *
 * 严格按照界面设计：
 *  - 左侧：SOC 大数字
 *  - 左下：温度 + 单体压差
 *  - 左下右侧：剩余容量
 *  - 右侧：电压 / 电流 / 功率 / 剩余里程
 *  - 底部：SOC 渐变条
 *
 * 仪表盘不显示“蓝牙已连接”等状态文字。
 */

namespace {
  static const uint16_t UI_PANEL       = 0x0B2F50;
  static const uint16_t UI_PANEL_DARK  = 0x08243F;
  static const uint16_t UI_WHITE       = TFT_WHITE;
  static const uint16_t UI_VOLTAGE     = TFT_YELLOW;
  static const uint16_t UI_CURRENT     = TFT_GREEN;
  static const uint16_t UI_POWER       = TFT_RED;
  static const uint16_t UI_RANGE       = TFT_CYAN;
  static const uint16_t UI_TEMP        = TFT_GREEN;

  static const float SOC_WARN_THRESHOLD = 30.0f;
  static const float SOC_CRITICAL_THRESHOLD = 15.0f;

  static uint16_t lerp565(uint16_t a, uint16_t b, uint16_t t) {
    uint8_t ar=(a>>11)&0x1F, ag=(a>>5)&0x3F, ab=a&0x1F;
    uint8_t br=(b>>11)&0x1F, bg=(b>>5)&0x3F, bb=b&0x1F;
    uint8_t rr=ar+((br-ar)*t)/100;
    uint8_t rg=ag+((bg-ag)*t)/100;
    uint8_t rb=ab+((bb-ab)*t)/100;
    return (rr<<11)|(rg<<5)|rb;
  }

  static uint16_t socColor(float soc) {
    if (soc <= SOC_CRITICAL_THRESHOLD) return TFT_RED;
    if (soc <= SOC_WARN_THRESHOLD) return TFT_YELLOW;
    return UI_WHITE;
  }

  static void drawPanel(TFT_eSprite& sprite, int x, int y, int w, int h) {
    sprite.fillRoundRect(x, y, w, h, 7, UI_PANEL);
  }

  static void drawMetricRow(TFT_eSprite& sprite,
                            char icon,
                            const char* label,
                            const String& value,
                            uint16_t color) {
    sprite.fillSprite(TFT_BLACK);
    sprite.fillRoundRect(0, 0, 168, 27, 7, UI_PANEL);

    // 左侧圆形 V/A/W 标识
    sprite.drawCircle(15, 13, 12, color);
    sprite.setTextColor(color, UI_PANEL);
    sprite.drawCentreString(String(icon), 15, 1, 4);

    FontGB2312::drawText(sprite, 31, 5, String(label), color, UI_PANEL, 1);

    sprite.setTextColor(color, UI_PANEL);
    sprite.drawRightString(value, 164, 4, 2);
  }
}

bool Display::changed(float a, float b, float eps) const {
  return fabsf(a-b) >= eps;
}

void Display::begin() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft_.init();
  tft_.setRotation(1);
  tft_.fillScreen(TFT_BLACK);

  socSprite_.setColorDepth(16);
  leftInfoSprite_.setColorDepth(16);
  rowSprite_.setColorDepth(16);
  barSprite_.setColorDepth(16);

  socSprite_.createSprite(140, 78);
  leftInfoSprite_.createSprite(140, 67);
  rowSprite_.createSprite(168, 27);
  barSprite_.createSprite(304, 9);

  socSprite_.fillSprite(TFT_BLACK);
  leftInfoSprite_.fillSprite(TFT_BLACK);
  rowSprite_.fillSprite(TFT_BLACK);
  barSprite_.fillSprite(TFT_BLACK);

  initialized_ = true;
  firstDashboard_ = true;
  drawFullPage(g_bmsData);
}

void Display::update(const BmsData& d) {
  if (!initialized_) return;

  if (d.bootState != lastBootState_ || d.hotspot != lastData_.hotspot) {
    drawFullPage(d);
    lastBootState_ = d.bootState;
    lastData_ = d;
    return;
  }

  if (d.bootState != BOOT_CONNECTED && !d.online) {
    if (d.statusMessage != lastData_.statusMessage ||
        d.scanAttempt != lastData_.scanAttempt ||
        d.hotspotIp != lastData_.hotspotIp ||
        d.mac != lastData_.mac) {
      drawFullPage(d);
      lastData_ = d;
    }
    return;
  }

  drawDashboard(d, firstDashboard_);
  firstDashboard_ = false;
  lastData_ = d;
}

void Display::drawFullPage(const BmsData& d) {
  tft_.fillScreen(TFT_BLACK);
  firstDashboard_ = (d.online || d.bootState == BOOT_CONNECTED);

  if (d.bootState == BOOT_SCANNING || d.bootState == BOOT_START) {
    TFT_eSprite page(&tft_);
    page.setColorDepth(16);
    page.createSprite(320,170);
    page.fillSprite(TFT_BLACK);

    FontGB2312::drawCenterString(page,160,7,"连接电池",TFT_CYAN,TFT_BLACK,2);
    FontGB2312::drawCenterString(page,160,43,
      d.statusMessage.length()?d.statusMessage:"扫描蓝牙...",
      TFT_WHITE,TFT_BLACK,1);

    int w=250;
    page.drawRoundRect(35,72,w,18,5,TFT_DARKGREY);
    int progress=(d.scanAttempt*100)/3;
    if(progress>100) progress=100;
    if(progress>0)
      page.fillRoundRect(38,75,(w-6)*progress/100,12,4,TFT_BLUE);

    FontGB2312::drawCenterString(page,160,96,
      String(d.scanAttempt)+"/3",TFT_YELLOW,TFT_BLACK,1);
    FontGB2312::drawCenterString(page,160,135,
      "自动扫描并连接JK保护板",TFT_LIGHTGREY,TFT_BLACK,1);
    page.pushSprite(0,0);
    page.deleteSprite();
    return;
  }

  if (d.bootState == BOOT_CONNECTING) {
    TFT_eSprite page(&tft_);
    page.setColorDepth(16);
    page.createSprite(320,170);
    page.fillSprite(TFT_BLACK);

    FontGB2312::drawCenterString(page,160,8,"正在连接",TFT_YELLOW,TFT_BLACK,2);
    FontGB2312::drawCenterString(page,160,50,
      d.mac.length()?d.mac:"JK-BMS",TFT_WHITE,TFT_BLACK,1);
    FontGB2312::drawCenterString(page,160,82,
      "正在建立蓝牙连接...",TFT_WHITE,TFT_BLACK,1);
    page.pushSprite(0,0);
    page.deleteSprite();
    return;
  }

  if ((d.bootState == BOOT_HOTSPOT || d.hotspot) && !d.online) {
    TFT_eSprite page(&tft_);
    page.setColorDepth(16);
    page.createSprite(320,170);
    page.fillSprite(TFT_BLACK);

    FontGB2312::drawCenterString(page,160,5,"热点设置",TFT_YELLOW,TFT_BLACK,2);
    FontGB2312::drawCenterString(page,160,43,
      "WiFi: JK-BMS-SETUP",TFT_WHITE,TFT_BLACK,1);
    FontGB2312::drawCenterString(page,160,70,
      "手机连接后打开网页",TFT_CYAN,TFT_BLACK,1);
    FontGB2312::drawCenterString(page,160,94,
      d.hotspotIp.length()?d.hotspotIp:"192.168.4.1",
      TFT_CYAN,TFT_BLACK,1);
    FontGB2312::drawCenterString(page,160,130,
      "扫描 / 选择 / 连接电池",TFT_LIGHTGREY,TFT_BLACK,1);
    page.pushSprite(0,0);
    page.deleteSprite();
    return;
  }

  drawDashboard(d, true);
}

void Display::drawDashboard(const BmsData& d, bool force) {
  if (force) {
    tft_.fillScreen(TFT_BLACK);
    drawSoc(d);
    drawVoltage(d);
    drawCurrent(d);
    drawPower(d);
    drawTemperature(d);
    drawRange(d);
    drawSocBar(d);
    return;
  }

  if (changed(d.soc,lastData_.soc,0.5f)) {
    drawSoc(d);
    drawSocBar(d);
  }

  if (changed(d.totalVoltage,lastData_.totalVoltage,0.1f))
    drawVoltage(d);

  if (changed(d.current,lastData_.current,0.1f))
    drawCurrent(d);

  if (changed(d.power,lastData_.power,1.0f))
    drawPower(d);

  if (changed(d.remainingCapacityAh,lastData_.remainingCapacityAh,0.1f) ||
      changed(d.temperature1,lastData_.temperature1,0.1f) ||
      changed(d.deltaCellVoltage,lastData_.deltaCellVoltage,0.001f))
    drawTemperature(d);

  if (changed(d.remainingRangeKm,lastData_.remainingRangeKm,0.1f))
    drawRange(d);
}

void Display::drawSoc(const BmsData& d) {
  socSprite_.fillSprite(TFT_BLACK);
  drawPanel(socSprite_, 0, 0, 140, 78);

  String value=String(d.soc,0);
  uint16_t color=socColor(d.soc);

  socSprite_.setTextColor(color,UI_PANEL);
  socSprite_.drawCentreString(value,70,0,7);

  socSprite_.setTextColor(color,UI_PANEL);
  socSprite_.drawString("%",108,47,4);

  socSprite_.pushSprite(4,4);
}

void Display::drawTemperature(const BmsData& d) {
  leftInfoSprite_.fillSprite(TFT_BLACK);

  // 左下：温度 + 单体压差
  leftInfoSprite_.fillRoundRect(0,0,73,67,7,UI_PANEL);
  String temp=String(d.temperature1,1)+"C";
  String delta=String(d.deltaCellVoltage*1000.0f,0)+"mV";

  leftInfoSprite_.setTextColor(UI_TEMP,UI_PANEL);
  leftInfoSprite_.drawString(temp,4,3,2);
  leftInfoSprite_.drawString(delta,4,31,2);

  // 右侧：剩余容量
  leftInfoSprite_.fillRoundRect(75,0,65,67,7,UI_PANEL);
  leftInfoSprite_.setTextColor(UI_WHITE,UI_PANEL);
  leftInfoSprite_.drawCentreString("Ah",107,1,2);

  String cap=String(d.remainingCapacityAh,1);
  leftInfoSprite_.drawCentreString(cap,107,25,4);

  leftInfoSprite_.pushSprite(4,84);
}

void Display::clearRow() {
  rowSprite_.fillSprite(TFT_BLACK);
}

void Display::drawVoltage(const BmsData& d) {
  drawMetricRow(rowSprite_,'V',"电压",String(d.totalVoltage,2)+"V",UI_VOLTAGE);
  rowSprite_.pushSprite(148,4);
}

void Display::drawCurrent(const BmsData& d) {
  drawMetricRow(rowSprite_,'A',"电流",String(d.current,1)+"A",UI_CURRENT);
  rowSprite_.pushSprite(148,33);
}

void Display::drawPower(const BmsData& d) {
  drawMetricRow(rowSprite_,'W',"功率",String(d.power,0)+"W",UI_POWER);
  rowSprite_.pushSprite(148,62);
}

void Display::drawRange(const BmsData& d) {
  rowSprite_.fillSprite(TFT_BLACK);
  rowSprite_.fillRoundRect(0,0,168,27,7,UI_PANEL);

  FontGB2312::drawText(rowSprite_,8,5,"剩余里程",UI_RANGE,UI_PANEL,1);
  rowSprite_.setTextColor(UI_RANGE,UI_PANEL);
  rowSprite_.drawRightString(String(d.remainingRangeKm,0)+" KM",164,4,2);

  rowSprite_.pushSprite(148,91);
}

void Display::drawSocBar(const BmsData& d) {
  barSprite_.fillSprite(TFT_BLACK);

  float ratio=d.soc/100.0f;
  if(ratio<0) ratio=0;
  if(ratio>1) ratio=1;

  const int w=304;
  const int segments=32;

  // UI 2.0：绿 -> 黄 -> 红渐变
  for(int i=0;i<segments;i++) {
    float p0=(float)i/segments;
    uint16_t color;

    if(p0<0.5f)
      color=lerp565(TFT_GREEN,TFT_YELLOW,(uint16_t)(p0*200));
    else
      color=lerp565(TFT_YELLOW,TFT_RED,(uint16_t)((p0-0.5f)*200));

    int sx=(w*i)/segments;
    int sw=(w*(i+1))/segments-(w*i)/segments;

    if ((i+1)/(float)segments <= ratio)
      barSprite_.fillRect(sx,0,sw,9,color);
    else
      barSprite_.fillRect(sx,0,sw,9,UI_PANEL_DARK);
  }

  barSprite_.drawRoundRect(0,0,w-1,8,3,TFT_DARKGREY);
  barSprite_.pushSprite(8,160);
}
