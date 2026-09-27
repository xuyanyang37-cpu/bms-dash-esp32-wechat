#ifndef JK_BLE_H
#define JK_BLE_H
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "BmsData.h"
#include "JkProtocol.h"
class JkBle{
public:
  JkBle();bool begin();bool scanAndConnect(uint32_t seconds=5);bool connected()const;void loop();
private:
  NimBLEClient* client_;NimBLERemoteCharacteristic* ch_;JkProtocol protocol_;uint8_t counter_;uint32_t lastRequest_;
  static JkBle* instance_;static void notifyCallback(NimBLERemoteCharacteristic*,uint8_t*,size_t,bool);
  void handleNotification(const uint8_t*,size_t);void request(uint8_t);
  bool connectDevice(NimBLEAdvertisedDevice*);
};
#endif
