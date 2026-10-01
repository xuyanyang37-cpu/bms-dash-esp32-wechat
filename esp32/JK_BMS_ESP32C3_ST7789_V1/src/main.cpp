#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include "esp_system.h"
#include "tft_setup.h"
#include "BmsData.h"
#include "JkBle.h"
#include "Display.h"
#include "WebConfig.h"

static JkBle jk;
static Display display;
static WebConfig webConfig;
static const char* AP_SSID="JK-BMS-SETUP";
static const char* AP_PASSWORD="12345678";

RTC_DATA_ATTR static uint8_t rtcFailedScanAttempts=0;

static void startHotspot(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASSWORD);
  IPAddress ip=WiFi.softAPIP();
  g_bmsData.hotspot=true;
  g_bmsData.hotspotIp=ip.toString();
  g_bmsData.bootState=BOOT_HOTSPOT;
  g_bmsData.statusMessage="等待网页设置";
  webConfig.begin(&jk);
  Serial.printf("HOTSPOT: %s %s heap=%u\\n",AP_SSID,ip.toString().c_str(),ESP.getFreeHeap());
}

void setup(){
  Serial.begin(115200);
  delay(50);
  esp_reset_reason_t resetReason=esp_reset_reason();
  Serial.println();
  Serial.printf("ESP32 reset reason: %d\\n",(int)resetReason);
  Serial.printf("RTC failed scans: %u\\n",(unsigned)rtcFailedScanAttempts);

  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,LOW);
  delay(20);
  g_bmsData.scanMax=3;
  g_bmsData.online=false;
  g_bmsData.valid=false;
  g_bmsData.hotspot=false;

  if(rtcFailedScanAttempts>=3){
    g_bmsData.scanAttempt=3;
    g_bmsData.bootState=BOOT_HOTSPOT;
    g_bmsData.statusMessage="三次连接失败，进入配网";
    display.begin();
    display.update(g_bmsData);
    delay(100);

    // 重要：如果是“重启后直接进入配网”路径，前面没有执行 jk.begin()。
    // 网页端扫描/连接会调用 NimBLE，因此这里必须先初始化 BLE。
    // 只初始化一次，不重复创建 Client。
    jk.begin();

    startHotspot();
    display.update(g_bmsData);
    return;
  }

  g_bmsData.bootState=BOOT_SCANNING;
  g_bmsData.scanAttempt=(uint8_t)(rtcFailedScanAttempts+1);
  g_bmsData.statusMessage="扫描蓝牙电池 "+String(g_bmsData.scanAttempt)+"/3";
  display.begin();
  display.update(g_bmsData);
  delay(300);
  jk.begin();

  bool connectedOk=false;
  uint8_t firstAttempt=(uint8_t)(rtcFailedScanAttempts+1);
  if(firstAttempt>3) firstAttempt=3;

  for(uint8_t attempt=firstAttempt;attempt<=3 && !connectedOk;attempt++){
    g_bmsData.scanAttempt=attempt;
    g_bmsData.scanMax=3;
    g_bmsData.bootState=BOOT_SCANNING;
    g_bmsData.online=false;
    g_bmsData.valid=false;
    g_bmsData.statusMessage="扫描蓝牙电池 "+String(attempt)+"/3";
    display.update(g_bmsData);
    delay(30);

    bool attemptOk=jk.scanAndConnect(5,attempt);
    if(attemptOk && jk.connected()){
      uint32_t verifyStart=millis();
      while(jk.connected() && !g_bmsData.valid && millis()-verifyStart<4000UL){
        jk.loop();
        delay(20);
      }
    }

    connectedOk=attemptOk && jk.connected() && g_bmsData.valid;
    if(connectedOk){
      rtcFailedScanAttempts=0;
      g_bmsData.bootState=BOOT_CONNECTED;
      g_bmsData.online=true;
      g_bmsData.statusMessage="已连接JK电池";
      display.update(g_bmsData);
    }else{
      g_bmsData.online=false;
      g_bmsData.bootState=BOOT_SCANNING;
      rtcFailedScanAttempts=attempt;
      g_bmsData.statusMessage="第 "+String(attempt)+"/3 次连接失败";
      display.update(g_bmsData);
      if(attempt<3) delay(300);
    }
  }

  if(!connectedOk){
    rtcFailedScanAttempts=3;
    startHotspot();
    display.update(g_bmsData);
  }
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
