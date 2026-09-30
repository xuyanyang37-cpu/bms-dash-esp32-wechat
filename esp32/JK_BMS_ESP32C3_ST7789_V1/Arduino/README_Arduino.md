# JK_BMS_ESP32C3_ST7789_V1 - Arduino IDE

Arduino IDE 入口工程。

打开本目录中的 `JK_BMS_ESP32C3_ST7789_V1_Arduino.ino`。

源码继续使用上一级 `src/` 中的现有代码，因此不会产生第二套 JK 协议/UI 源码。

需要安装：
- ESP32 Arduino Core
- NimBLE-Arduino 2.x
- TFT_eSPI 2.5.x

开发板：
ESP32C3 Dev Module

ST7789：
BL=5, CS=3, DC=2, RST=10, MOSI=7, SCLK=6

注意：当前 TFT 初始化顺序保持：
pinMode(TFT_BL, OUTPUT);
digitalWrite(TFT_BL, HIGH);
tft_.init();
tft_.setRotation(1);
