#ifndef JK_PROTOCOL_H
#define JK_PROTOCOL_H
#include <Arduino.h>
#include "BmsData.h"
#include "../JkConfig.h"
class JkProtocol{
public:
  static const size_t FRAME_MAX=360;
  JkProtocol();
  bool parseFrame(const uint8_t*,size_t,BmsData&);
  void buildCommand(uint8_t,uint8_t,uint8_t[20]);
private:
  static uint16_t u16le(const uint8_t*); static uint32_t u32le(const uint8_t*);
  static int16_t s16le(const uint8_t*); static bool validCrc(const uint8_t*,size_t);
  int detectOffset(const uint8_t*,size_t)const;
  bool parseMainFrame(const uint8_t*,size_t,BmsData&);
};
#endif
