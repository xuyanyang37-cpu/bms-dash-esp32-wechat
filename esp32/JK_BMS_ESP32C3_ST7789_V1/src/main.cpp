#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include "esp_system.h"
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
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASSWORD);
  IPAddress ip=WiFi.softAPIP();

  g_bmsData.hotspot=true;
  g_bmsData.hotspotIp=ip.toString();
  g_bmsData.bootState=BOOT_HOTSPOT;
  g_bmsData.statusMessage="等待网页设置";
  webConfig.begin(&jk);

// Serial.print("热点SSID: ");
  //Serial.println(AP_SSID);
  //Serial.print("热点IP: ");
// Serial.println(ip);
}

void setup(){
  // 记录上一次复位原因。若扫描过程中再次整机重启，
  // 串口会明确显示是 Brownout / Watchdog / Panic 等原因。
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.print("ESP32 reset reason: ");
  Serial.println((int)esp_reset_reason());

  // 背光脚必须在最早阶段明确拉低，避免 GPIO5 上电悬空导致背光闪烁。
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);

  // 等待电源/IO 稳定后再初始化 ST7789。
  delay(20);

  //Serial.println("ESP32-C3 JK BMS + ST7789 UI 2.0");
  //Serial.println("BL=5 CS=3 DC=2 RES=10 SDA=7 SCL=6");

  display.begin();
  jk.begin();

  // 开机最多自动扫描三次；三次都失败后进入AP热点设置模式。
  // 注意：scanAndConnect() 内部的 BLE 扫描是阻塞式的，因此在调用它之前
  // 必须先刷新一次屏幕，否则屏幕会一直停在初始化时的 0/3。
  bool ok=false;
  for(uint8_t i=0;i<3 && !ok;i++){
    uint8_t attempt=(uint8_t)(i+1);

    g_bmsData.scanAttempt=attempt;
    g_bmsData.scanMax=3;
    g_bmsData.bootState=BOOT_SCANNING;
    g_bmsData.online=false;
    g_bmsData.statusMessage="扫描蓝牙电池 " + String(attempt) + "/3";

    // 立即显示本次扫描编号，再进入最多 5 秒的BLE扫描。
    display.update(g_bmsData);
    delay(30);

    ok=jk.scanAndConnect(5, attempt);

    // 扫描/连接结束后立即刷新结果，不必等主循环的500ms周期。
    display.update(g_bmsData);

    if(!ok) delay(300);
  }

  if(!ok) startHotspot();
}

void loop(){
  jk.loop();
  webConfig.loop();

  // UI 2.0 按设计目标以 500ms 刷新检查一次；
  // Display 内部仍采用局部脏区刷新，只有数据变化才真正写屏。
  static uint32_t drawMs=0;
  if(millis()-drawMs>=500){
    drawMs=millis();
    display.update(g_bmsData);
  }

  delay(5);
}
