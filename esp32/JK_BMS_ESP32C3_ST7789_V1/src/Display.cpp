#include "Display.h"
#include "FontGB2312.h"
#include <math.h>

/*
 * JK BMS + ST7789 1.9" / 320x170
 * UI 2.0 - 按设计图重新做像素级布局。
 *
 * 主界面：
 *   左：SOC
 *   左下：温度 / 单体压差 + 剩余容量
 *   右：电压 / 电流 / 功率 / 剩余里程
 *   底：SOC 渐变条
 *
 * 主界面不显示“蓝牙已连接”等状态文字。
 */

namespace {
  // 设计图的深蓝色圆角卡片。
  static const uint16_t UI_PANEL      = 0x0948;
  static const uint16_t UI_PANEL_DARK = 0x0127;

  static const uint16_t UI_WHITE   = TFT_WHITE;
  static const uint16_t UI_VOLTAGE = 0xFFE0; // 明黄色
  static const uint16_t UI_CURRENT = 0x07E0; // 亮绿色
  static const uint16_t UI_POWER   = 0xFCA8; // 柔和红色
  static const uint16_t UI_RANGE   = 0x2F3C; // 青色
  static const uint16_t UI_TEMP    = 0x07E0;

  // 设计图：低电量可调颜色阈值。
  static const float SOC_WARN_THRESHOLD = 30.0f;
  static const float SOC_CRITICAL_THRESHOLD = 15.0f;

  // 16bit RGB565 颜色线性插值。
  static uint16_t lerp565(uint16_t a, uint16_t b, uint16_t percent) {
    if (percent > 100) percent = 100;

    uint8_t ar = (a >> 11) & 0x1F;
    uint8_t ag = (a >> 5)  & 0x3F;
    uint8_t ab = a & 0x1F;

    uint8_t br = (b >> 11) & 0x1F;
    uint8_t bg = (b >> 5)  & 0x3F;
    uint8_t bb = b & 0x1F;

    uint8_t rr = ar + ((int16_t)(br - ar) * percent) / 100;
    uint8_t rg = ag + ((int16_t)(bg - ag) * percent) / 100;
    uint8_t rb = ab + ((int16_t)(bb - ab) * percent) / 100;

    return ((uint16_t)rr << 11) | ((uint16_t)rg << 5) | rb;
  }

  static uint16_t socColor(float soc) {
    if (soc <= SOC_CRITICAL_THRESHOLD) return TFT_RED;
    if (soc <= SOC_WARN_THRESHOLD) return TFT_YELLOW;
    return UI_WHITE;
  }

  static void drawPanel(TFT_eSprite& sprite,
                        int16_t x, int16_t y,
                        int16_t w, int16_t h) {
    sprite.fillRoundRect(x, y, w, h, 7, UI_PANEL);
  }

  /*
   * 右侧数据卡片：
   * 168 x 27 px
   * 圆角 7 px
   * 图标圆直径约 24 px
   * 中文 16 px
   * 数值 16 px
   */
  static void drawMetricRow(TFT_eSprite& sprite,
                            char icon,
                            const char* label,
                            const String& value,
                            uint16_t color) {
    sprite.fillSprite(TFT_BLACK);
    sprite.fillRoundRect(0, 0, 168, 27, 7, UI_PANEL);

    // 图标圆：24 px，字母使用 Font 2，避免 Font 4 超出圆形。
    sprite.drawCircle(15, 13, 11, color);
    sprite.setTextColor(color, UI_PANEL);
    sprite.drawCentreString(String(icon), 15, 5, 2);

    // 中文 16x16 点阵，与设计图的数据标签高度一致。
    FontGB2312::drawText(sprite, 31, 5,
                         String(label), color, UI_PANEL, 1);

    // 右对齐数值，避免电流/功率变长时挤压中文。
    sprite.setTextColor(color, UI_PANEL);
    sprite.drawRightString(value, 164, 5, 2);
  }
}

bool Display::changed(float a, float b, float eps) const {
  return fabsf(a - b) >= eps;
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

  // 与设计图的比例对应。
  socSprite_.createSprite(140, 78);
  leftInfoSprite_.createSprite(140, 67);
  rowSprite_.createSprite(168, 27);

  // 底部渐变条横跨左右两个区域。
  barSprite_.createSprite(312, 11);

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

  if (d.bootState != lastBootState_ ||
      d.hotspot != lastData_.hotspot) {
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

  if (d.bootState == BOOT_SCANNING ||
      d.bootState == BOOT_START) {
    TFT_eSprite page(&tft_);
    page.setColorDepth(16);
    page.createSprite(320, 170);
    page.fillSprite(TFT_BLACK);

    FontGB2312::drawCenterString(page, 160, 7,
                                 "连接电池",
                                 TFT_CYAN, TFT_BLACK, 2);
    FontGB2312::drawCenterString(
        page, 160, 43,
        d.statusMessage.length() ? d.statusMessage : "扫描蓝牙...",
        TFT_WHITE, TFT_BLACK, 1);

    int w = 250;
    page.drawRoundRect(35, 72, w, 18, 5, TFT_DARKGREY);

    int progress = (d.scanAttempt * 100) / 3;
    if (progress > 100) progress = 100;

    if (progress > 0) {
      page.fillRoundRect(38, 75,
                         (w - 6) * progress / 100,
                         12, 4, TFT_BLUE);
    }

    FontGB2312::drawCenterString(page, 160, 96,
                                 String(d.scanAttempt) + "/3",
                                 TFT_YELLOW, TFT_BLACK, 1);
    FontGB2312::drawCenterString(page, 160, 135,
                                 "自动扫描并连接JK保护板",
                                 TFT_LIGHTGREY, TFT_BLACK, 1);

    page.pushSprite(0, 0);
    page.deleteSprite();
    return;
  }

  if (d.bootState == BOOT_CONNECTING) {
    TFT_eSprite page(&tft_);
    page.setColorDepth(16);
    page.createSprite(320, 170);
    page.fillSprite(TFT_BLACK);

    FontGB2312::drawCenterString(page, 160, 8,
                                 "正在连接",
                                 TFT_YELLOW, TFT_BLACK, 2);
    FontGB2312::drawCenterString(page, 160, 50,
                                 d.mac.length() ? d.mac : "JK-BMS",
                                 TFT_WHITE, TFT_BLACK, 1);
    FontGB2312::drawCenterString(page, 160, 82,
                                 "正在建立蓝牙连接...",
                                 TFT_WHITE, TFT_BLACK, 1);

    page.pushSprite(0, 0);
    page.deleteSprite();
    return;
  }

  if ((d.bootState == BOOT_HOTSPOT || d.hotspot) &&
      !d.online) {
    TFT_eSprite page(&tft_);
    page.setColorDepth(16);
    page.createSprite(320, 170);
    page.fillSprite(TFT_BLACK);

    FontGB2312::drawCenterString(page, 160, 5,
                                 "热点设置",
                                 TFT_YELLOW, TFT_BLACK, 2);
    FontGB2312::drawCenterString(page, 160, 43,
                                 "WiFi: JK-BMS-SETUP",
                                 TFT_WHITE, TFT_BLACK, 1);
    FontGB2312::drawCenterString(page, 160, 70,
                                 "手机连接后打开网页",
                                 TFT_CYAN, TFT_BLACK, 1);
    FontGB2312::drawCenterString(
        page, 160, 94,
        d.hotspotIp.length() ? d.hotspotIp : "192.168.4.1",
        TFT_CYAN, TFT_BLACK, 1);
    FontGB2312::drawCenterString(page, 160, 130,
                                 "扫描 / 选择 / 连接电池",
                                 TFT_LIGHTGREY, TFT_BLACK, 1);

    page.pushSprite(0, 0);
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

  // SOC 变化：数字 + 底部渐变条同时刷新。
  if (changed(d.soc, lastData_.soc, 0.5f)) {
    drawSoc(d);
    drawSocBar(d);
  }

  if (changed(d.totalVoltage, lastData_.totalVoltage, 0.1f)) {
    drawVoltage(d);
  }

  if (changed(d.current, lastData_.current, 0.1f)) {
    drawCurrent(d);
  }

  if (changed(d.power, lastData_.power, 1.0f)) {
    drawPower(d);
  }

  if (changed(d.remainingCapacityAh, lastData_.remainingCapacityAh, 0.1f) ||
      changed(d.temperature1, lastData_.temperature1, 0.1f) ||
      changed(d.deltaCellVoltage, lastData_.deltaCellVoltage, 0.001f)) {
    drawTemperature(d);
  }

  if (changed(d.remainingRangeKm, lastData_.remainingRangeKm, 0.1f)) {
    drawRange(d);
  }
}

void Display::drawSoc(const BmsData& d) {
  socSprite_.fillSprite(TFT_BLACK);

  // 4,4 -> 144,82；与左下区域保持 2 px 间距。
  drawPanel(socSprite_, 0, 0, 140, 78);

  String value = String(d.soc, 0);
  uint16_t color = socColor(d.soc);

  // Font 7 是 TFT_eSPI 原生 48 px 七段数字，专门支持数字。
  // 设计图的大 SOC 数字采用这一字体最接近。
  socSprite_.setTextColor(color, UI_PANEL);
  socSprite_.drawCentreString(value, 70, -1, 7);

  // % 小号、右下角。
  socSprite_.setTextColor(color, UI_PANEL);
  socSprite_.drawString("%", 108, 47, 4);

  socSprite_.pushSprite(4, 4);
}

void Display::drawTemperature(const BmsData& d) {
  leftInfoSprite_.fillSprite(TFT_BLACK);

  // 左卡片：73 x 67。
  leftInfoSprite_.fillRoundRect(0, 0, 73, 67, 7, UI_PANEL);

  String temp = String(d.temperature1, 1) + "C";
  String delta = String(d.deltaCellVoltage * 1000.0f, 0) + "mV";

  // 16 px 字体，两行，和设计图的信息密度一致。
  leftInfoSprite_.setTextColor(UI_TEMP, UI_PANEL);
  leftInfoSprite_.drawString(temp, 4, 3, 2);
  leftInfoSprite_.drawString(delta, 4, 31, 2);

  // 右卡片：65 x 67。
  leftInfoSprite_.fillRoundRect(75, 0, 65, 67, 7, UI_PANEL);

  leftInfoSprite_.setTextColor(UI_WHITE, UI_PANEL);
  leftInfoSprite_.drawCentreString("Ah", 107, 1, 2);

  String cap = String(d.remainingCapacityAh, 1);
  leftInfoSprite_.drawCentreString(cap, 107, 24, 4);

  leftInfoSprite_.pushSprite(4, 84);
}

void Display::drawVoltage(const BmsData& d) {
  drawMetricRow(rowSprite_, 'V', "电压",
                String(d.totalVoltage, 2) + "V",
                UI_VOLTAGE);
  rowSprite_.pushSprite(148, 4);
}

void Display::drawCurrent(const BmsData& d) {
  drawMetricRow(rowSprite_, 'A', "电流",
                String(d.current, 1) + "A",
                UI_CURRENT);
  rowSprite_.pushSprite(148, 33);
}

void Display::drawPower(const BmsData& d) {
  drawMetricRow(rowSprite_, 'W', "功率",
                String(d.power, 0) + "W",
                UI_POWER);
  rowSprite_.pushSprite(148, 62);
}

void Display::drawRange(const BmsData& d) {
  rowSprite_.fillSprite(TFT_BLACK);
  rowSprite_.fillRoundRect(0, 0, 168, 27, 7, UI_PANEL);

  FontGB2312::drawText(rowSprite_, 8, 5,
                       "剩余里程",
                       UI_RANGE, UI_PANEL, 1);

  rowSprite_.setTextColor(UI_RANGE, UI_PANEL);
  rowSprite_.drawRightString(
      String(d.remainingRangeKm, 0) + " KM",
      164, 5, 2);

  rowSprite_.pushSprite(148, 91);
}

void Display::drawSocBar(const BmsData& d) {
  barSprite_.fillSprite(TFT_BLACK);

  float ratio = d.soc / 100.0f;
  if (ratio < 0.0f) ratio = 0.0f;
  if (ratio > 1.0f) ratio = 1.0f;

  const int16_t w = 312;
  const int16_t h = 11;

  /*
   * 设计图是连续绿 -> 黄 -> 红渐变。
   * 按像素计算，而不是 32 段，避免在小屏上看到明显色块。
   */
  int filled = (int)(w * ratio + 0.5f);

  for (int16_t x = 0; x < w; ++x) {
    if (x >= filled) {
      barSprite_.drawFastVLine(x, 0, h, UI_PANEL_DARK);
      continue;
    }

    uint16_t color;

    if (w <= 1) {
      color = TFT_GREEN;
    } else {
      uint16_t p = (uint16_t)((x * 100L) / (w - 1));

      if (p <= 50) {
        color = lerp565(TFT_GREEN, TFT_YELLOW,
                        (uint16_t)(p * 2));
      } else {
        color = lerp565(TFT_YELLOW, TFT_RED,
                        (uint16_t)((p - 50) * 2));
      }
    }

    barSprite_.drawFastVLine(x, 0, h, color);
  }

  // 细边框 + 3 px 圆角，保持设计图的胶囊形效果。
  barSprite_.drawRoundRect(0, 0, w, h, 3, TFT_DARKGREY);

  barSprite_.pushSprite(4, 157);
}
