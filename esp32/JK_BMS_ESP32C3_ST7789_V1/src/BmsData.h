#ifndef BMS_DATA_H
#define BMS_DATA_H
#include <Arduino.h>
#define JK_MAX_CELLS 32
struct BmsData {
  bool online=false, valid=false;
  uint8_t cellCount=0;
  float totalVoltage=0, current=0, power=0, soc=0;
  float minCellVoltage=0, maxCellVoltage=0, deltaCellVoltage=0;
  uint8_t minCell=0, maxCell=0;
  float cellVoltage[JK_MAX_CELLS]={0};
  float temperature1=0, temperature2=0, mosTemperature=0;
  bool charging=false, discharging=false, balancing=false, heating=false;
  uint32_t errors=0, updateMs=0;
  String mac, deviceName, softwareVersion, hardwareVersion;
};
extern BmsData g_bmsData;
#endif
