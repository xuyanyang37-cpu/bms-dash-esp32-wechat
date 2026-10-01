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

  // 在显示器初始化之前就明确进入“扫描”状态。
  g_bmsData.bootState=BOOT_SCANNING;
  g_bmsData.scanAttempt=1;
  g_bmsData.scanMax=3;
  g_bmsData.online=false;
  g_bmsData.valid=false;
  g_bmsData.statusMessage="扫描蓝牙电池 1/3";

  display.begin();

  // 开机扫描页先稳定显示，再初始化 BLE。
  display.update(g_bmsData);
  delay(300);

  jk.begin();

  // 开机最多自动扫描三次；三次都失败后进入AP热点设置模式。
  // connectedOk 是整个 setup() 的唯一“蓝牙已确认连接”状态。
  bool connectedOk=false;

  for(uint8_t i=0;i<3 && !connectedOk;i++){
    uint8_t attempt=(uint8_t)(i+1);

    g_bmsData.scanAttempt=attempt;
    g_bmsData.scanMax=3;
    g_bmsData.bootState=BOOT_SCANNING;
    g_bmsData.online=false;
    g_bmsData.statusMessage="扫描蓝牙电池 " + String(attempt) + "/3";

    // 立即显示本次扫描编号，再进入最多 5 秒的BLE扫描。
    display.update(g_bmsData);
    delay(30);

    bool attemptOk=jk.scanAndConnect(5, attempt);

    // “BLE/GATT 已连接”还不能证明这是一个真正可通信的 JK BMS。
    // 必须继续等待 JK 的有效主数据帧；JkProtocol::parseFrame()
    // 成功后才会把 g_bmsData.valid 置为 true。
    // 这样可以避免连接到错误的 BLE 设备、服务存在但协议不匹配，
    // 或通知没有正常工作的设备后直接误进入主界面。
    if(attemptOk && jk.connected()){
      uint32_t verifyStart=millis();
      while(jk.connected() &&
            !g_bmsData.valid &&
            millis()-verifyStart<4000UL){
        jk.loop();
        delay(20);
      }
    }

    // 真正成功必须同时满足：
    // 1. BLE 客户端仍然连接
    // 2. 已收到并校验通过 JK 有效数据帧
    connectedOk=attemptOk && jk.connected() && g_bmsData.valid;

    if(connectedOk){
      g_bmsData.bootState=BOOT_CONNECTED;
      g_bmsData.online=true;
      g_bmsData.statusMessage="已连接JK电池";

      // 只有这里才允许从扫描/连接页切换到主仪表盘。
      display.update(g_bmsData);
    }else{
      g_bmsData.online=false;
      g_bmsData.bootState=BOOT_SCANNING;

      // 本次连接失败，明确显示失败后再进入下一次扫描。
      g_bmsData.statusMessage="第 " + String(attempt) + "/3 次连接失败";
      display.update(g_bmsData);

      if(attempt < 3) delay(300);
    }
  }

  // 三次扫描/连接全部失败：禁止进入主界面，直接进入配网热点。
  if(!connectedOk){
    startHotspot();
    display.update(g_bmsData);
  }
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
