#include <Arduino.h>
#include "../tft_setup.h"
#include "BmsData.h"
#include "JkBle.h"
#include "Display.h"
BmsData g_bmsData;
static JkBle jk;static Display display;
void setup(){
  Serial.begin(115200);delay(500);
  Serial.println("ESP32-C3 JK BMS + ST7789 V1");
  Serial.println("BL=5 CS=3 DC=2 RES=10 SDA=7 SCL=6");
  display.begin();jk.begin();jk.scanAndConnect(6);
}
void loop(){
  jk.loop();
  static uint32_t drawMs=0,scanMs=0;
  if(millis()-drawMs>=200){drawMs=millis();display.update(g_bmsData);}
  if(!jk.connected()&&millis()-scanMs>=5000){scanMs=millis();jk.scanAndConnect(4);}
  delay(5);
}
