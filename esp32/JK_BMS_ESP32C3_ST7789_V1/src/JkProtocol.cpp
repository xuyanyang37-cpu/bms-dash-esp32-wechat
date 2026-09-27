#include "JkProtocol.h"
#include <string.h>
JkProtocol::JkProtocol(){}
uint16_t JkProtocol::u16le(const uint8_t*p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
uint32_t JkProtocol::u32le(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int16_t JkProtocol::s16le(const uint8_t*p){return (int16_t)u16le(p);}
bool JkProtocol::validCrc(const uint8_t*p,size_t n){if(n<5)return false;uint8_t s=0;for(size_t i=0;i+1<n;i++)s+=p[i];return s==p[n-1];}
int JkProtocol::detectOffset(const uint8_t*p,size_t n)const{
#if JK_PROTOCOL_32S
  return 32;
#else
  return 0;
#endif
}
bool JkProtocol::parseMainFrame(const uint8_t*p,size_t n,BmsData&o){
  int off=detectOffset(p,n);
  if(n<(size_t)(184+off))return false;
  uint8_t cells=JK_PROTOCOL_32S ? 32 : 24;
  float minV=100,maxV=0;uint8_t minC=0,maxC=0;
  for(uint8_t i=0;i<cells;i++){
    size_t pos=6+i*2;if(pos+1>=n)break;
    float v=u16le(p+pos)*0.001f;o.cellVoltage[i]=v;
    if(v>0.5f&&v<6.0f){if(v<minV){minV=v;minC=i+1;}if(v>maxV){maxV=v;maxC=i+1;}}
  }
  o.cellCount=cells;o.minCellVoltage=minV<100?minV:0;o.maxCellVoltage=maxV;
  o.deltaCellVoltage=(maxV>0&&minV<100)?maxV-minV:0;o.minCell=minC;o.maxCell=maxC;
  o.totalVoltage=u32le(p+118+off)*0.001f;
  o.current=(int32_t)u32le(p+126+off)*0.001f;
  o.power=o.totalVoltage*o.current;
  o.temperature1=s16le(p+130+off)*0.1f;
  o.temperature2=s16le(p+132+off)*0.1f;
  o.mosTemperature=s16le(p+(off?112+off:134))*0.1f;
  if(off)o.errors=u32le(p+134+off);else o.errors=u16le(p+136);
  o.balancing = p[169+off] != 0;
  o.charging = p[166+off] != 0;
  o.discharging = p[167+off] != 0;
  o.heating = p[183+off] != 0;
  o.soc = p[141+off];
  o.valid=o.totalVoltage>0.1f;o.online=o.valid;o.updateMs=millis();
  return o.valid;
}
bool JkProtocol::parseFrame(const uint8_t*p,size_t n,BmsData&o){
  if(n<6||p[0]!=0x55||p[1]!=0xAA||p[2]!=0xEB||p[3]!=0x90)return false;
  if(!validCrc(p,n))return false;
  if(p[4]==0x02)return parseMainFrame(p,n,o);
  return false;
}
void JkProtocol::buildCommand(uint8_t cmd,uint8_t counter,uint8_t out[20]){
  memset(out,0,20);out[0]=0xAA;out[1]=0x55;out[2]=0x90;out[3]=0xEB;out[4]=cmd;out[16]=counter;
  uint8_t s=0;for(int i=0;i<19;i++)s+=out[i];out[19]=s;
}
