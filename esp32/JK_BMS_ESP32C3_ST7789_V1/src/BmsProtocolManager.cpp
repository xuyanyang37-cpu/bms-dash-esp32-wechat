#include "BmsProtocolManager.h"

BmsProtocolManager::BmsProtocolManager()
  : activeProtocol_(nullptr) {
}

void BmsProtocolManager::begin(bool protocol32S) {
  jkProtocol_.setProtocol32S(protocol32S);
  activeProtocol_=&jkProtocol_;
}

void BmsProtocolManager::setProtocol32S(bool enable) {
  jkProtocol_.setProtocol32S(enable);
}

bool BmsProtocolManager::isProtocol32S() const {
  return jkProtocol_.isProtocol32S();
}

BmsProtocol* BmsProtocolManager::detectProtocol(const uint8_t* data, size_t len) {
  if(jkProtocol_.canHandle(data,len))
    return &jkProtocol_;

  return nullptr;
}

bool BmsProtocolManager::parseFrame(const uint8_t* data, size_t len, BmsData& out) {
  BmsProtocol* detected=detectProtocol(data,len);
  if(!detected)
    return false;

  activeProtocol_=detected;
  return activeProtocol_->parseFrame(data,len,out);
}

bool BmsProtocolManager::buildCommand(uint8_t command, uint8_t counter, uint8_t out[20]) {
  if(!activeProtocol_)
    activeProtocol_=&jkProtocol_;

  return activeProtocol_->buildCommand(command,counter,out);
}

int BmsProtocolManager::findFrameStart(const uint8_t* data, size_t len) {
  // 逐个协议询问帧头位置。
  // 以后增加协议后，优先返回最先出现的有效协议帧头。
  int start=jkProtocol_.findFrameStart(data,len);
  if(start>=0)
    return start;

  return -1;
}

size_t BmsProtocolManager::expectedFrameLength() const {
  return activeProtocol_ ? activeProtocol_->expectedFrameLength() : 300;
}

const char* BmsProtocolManager::protocolName() const {
  return activeProtocol_ ? activeProtocol_->name() : "NONE";
}
