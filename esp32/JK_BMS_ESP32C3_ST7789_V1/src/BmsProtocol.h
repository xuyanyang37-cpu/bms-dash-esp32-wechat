#ifndef BMS_PROTOCOL_H
#define BMS_PROTOCOL_H

#include <Arduino.h>
#include "BmsData.h"

// 所有保护板协议的统一接口。
// BLE 层只负责连接、Notify、Write 和原始字节缓冲。
class BmsProtocol {
public:
  virtual ~BmsProtocol() {}

  virtual const char* name() const = 0;

  // 判断帧头是否属于本协议。
  virtual bool canHandle(const uint8_t* data, size_t len) const = 0;

  // 在接收缓冲区寻找本协议帧头，找不到返回 -1。
  virtual int findFrameStart(const uint8_t* data, size_t len) const = 0;

  // 解析完整协议帧。
  virtual bool parseFrame(const uint8_t* data, size_t len, BmsData& out) = 0;

  // 构造发送命令。
  virtual bool buildCommand(uint8_t command, uint8_t counter, uint8_t out[20]) = 0;

  virtual size_t expectedFrameLength() const = 0;
};

#endif
