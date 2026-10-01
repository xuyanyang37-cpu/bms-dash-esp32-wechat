#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include "../tft_setup.h"
#include "BmsData.h"
#include "JkBle.h"
#include "Display.h"
#include "WebConfig.h"

static JkBle jk;
static Display display;
static WebConfig webConfig;

static const char* AP_SSID="JK-BMS-SETUP";
static const char* AP_PASSWORD="12345678";

static void startHotspot(){
  Serial.println("[HOTSPOT] ENTER");
  Serial.printf("[HOTSPOT] heap before WiFi=%u\\n", ESP.getFreeHeap());

  g_bmsData.hotspot=true;
  g_bmsData.bootState=BOOT_HOTSPOT;
  g_bmsData.statusMessage="等待网页设置";

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASSWORD);
  IPAddress ip=WiFi.softAPIP();
  g_bmsData.hotspotIp=ip.toString();

  Serial.printf("[HOTSPOT] IP=%s heap after WiFi=%u\\n",
                g_bmsData.hotspotIp.c_str(), ESP.getFreeHeap());

  // 先让LCD显示热点页面，再启动网页服务器。
  display.update(g_bmsData);
  Serial.printf("[HOTSPOT] heap after display=%u\\n", ESP.getFreeHeap());

  webConfig.begin(&jk);
  Serial.printf("[HOTSPOT] heap after WebConfig=%u\\n", ESP.getFreeHeap());
}

void setup(){
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("==============================");
  Serial.println("JK BMS START");
  Serial.printf("Free heap = %u\\n", ESP.getFreeHeap());
  Serial.println("==============================");

  // 最先锁定背光为常亮，后续程序绝不再切换 GPIO5。
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  delay(100);

  display.begin();
  jk.begin();

  // 开机最多自动扫描三次；三次都失败后进入AP热点设置模式
  bool ok=false;
  for(uint8_t i=0;i<3 && !ok;i++){
    ok=jk.scanAndConnect(5);
    if(!ok) delay(300);
  }

  if(!ok) startHotspot();
}

void loop(){
  jk.loop();
  webConfig.loop();

  static uint32_t drawMs=0;
  if(millis()-drawMs>=500){
    drawMs=millis();
    display.update(g_bmsData);
  }

  delay(5);
}
