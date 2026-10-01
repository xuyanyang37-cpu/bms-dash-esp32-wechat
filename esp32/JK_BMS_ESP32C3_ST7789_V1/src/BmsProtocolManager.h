#ifndef BMS_PROTOCOL_MANAGER_H
#define BMS_PROTOCOL_MANAGER_H

#include <Arduino.h>
#include "BmsData.h"
#include "BmsProtocol.h"
#include "JkProtocol.h"

// 协议管理层。
// 当前注册 JK；以后增加 Ant/JBD/DALY 等协议时，
// 只在这里增加协议对象和检测规则，BLE/UI 不需要重写。
class BmsProtocolManager {
public:
  BmsProtocolManager();

  void begin(bool protocol32S);
  void setProtocol32S(bool enable);
  bool isProtocol32S() const;

  bool parseFrame(const uint8_t* data, size_t len, BmsData& out);
  bool buildCommand(uint8_t command, uint8_t counter, uint8_t out[20]);

  // 通用帧同步接口，BLE 层不再写死 JK 帧头。
  int findFrameStart(const uint8_t* data, size_t len);
  size_t expectedFrameLength() const;

  const char* protocolName() const;

private:
  JkProtocol jkProtocol_;
  BmsProtocol* activeProtocol_;

  BmsProtocol* detectProtocol(const uint8_t* data, size_t len);
};

#endif
