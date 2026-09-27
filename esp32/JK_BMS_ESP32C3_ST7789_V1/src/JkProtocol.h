#ifndef JK_PROTOCOL_H
#define JK_PROTOCOL_H
#include <Arduino.h>
#include "BmsData.h"
class JkProtocol {
public:
  static const size_t FRAME_MAX=360;
  JkProtocol();
  bool parseFrame(const uint8_t* frame,size_t length,BmsData& out);
  void buildCommand(uint8_t command,uint8_t counter,uint8_t out[20]);
private:
  static uint16_t u16le(const uint8_t*p);
  static uint32_t u32le(const uint8_t*p);
  static int16_t s16le(const uint8_t*p);
  static bool validCrc(const uint8_t*p,size_t length);
  int detectOffset(const uint8_t*p,size_t length) const;
  bool parseMainFrame(const uint8_t*p,size_t length,BmsData&out);
};
#endif
