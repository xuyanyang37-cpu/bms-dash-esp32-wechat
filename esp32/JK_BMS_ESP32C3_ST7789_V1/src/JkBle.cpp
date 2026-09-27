#include "JkBle.h"
JkBle* JkBle::instance_=nullptr;
static const char* SERVICE="FFE0";static const char* CHAR="FFE1";
JkBle::JkBle():client_(nullptr),ch_(nullptr),counter_(0),lastRequest_(0){instance_=this;}
bool JkBle::begin(){NimBLEDevice::init("JK-C3-DISPLAY");NimBLEDevice::setPower(9);return true;}
bool JkBle::scanAndConnect(uint32_t sec){
  if(connected())return true;NimBLEScan*s=NimBLEDevice::getScan();s->setActiveScan(true);
  NimBLEScanResults r=s->getResults(sec*1000,false);NimBLEAdvertisedDevice*best=nullptr;int br=-127;
  for(int i=0;i<r.getCount();i++){NimBLEAdvertisedDevice*d=r.getDevice(i);String n=d->getName().c_str();n.toUpperCase();
    bool ok=d->isAdvertisingService(NimBLEUUID(SERVICE))||n.indexOf("JK")>=0||n.indexOf("JIKONG")>=0||n.indexOf("BMS")>=0;
    if(ok&&d->getRSSI()>br){br=d->getRSSI();best=d;}}
  if(!best)return false;return connectDevice(best);
}
bool JkBle::connectDevice(NimBLEAdvertisedDevice*d){
  if(client_){client_->disconnect();NimBLEDevice::deleteClient(client_);client_=nullptr;}
  client_=NimBLEDevice::createClient();if(!client_)return false;
  client_->setConnectTimeout(5000);if(!client_->connect(d->getAddress()))return false;
  NimBLERemoteService*s=client_->getService(SERVICE);if(!s)return false;
  ch_=s->getCharacteristic(CHAR);if(!ch_)return false;
  if(!ch_->canNotify()||!ch_->subscribe(true,notifyCallback))return false;
  g_bmsData.mac=d->getAddress().toString().c_str();g_bmsData.deviceName=d->getName().c_str();g_bmsData.online=true;
  request(0x96);delay(100);request(0x97);lastRequest_=millis();return true;
}
void JkBle::notifyCallback(NimBLERemoteCharacteristic*,uint8_t*d,size_t n,bool){if(instance_)instance_->handleNotification(d,n);}
void JkBle::handleNotification(const uint8_t*d,size_t n){
  static uint8_t rx[700];static size_t len=0;if(n>sizeof(rx)||len+n>sizeof(rx)){len=0;return;}
  memcpy(rx+len,d,n);len+=n;
  while(len>=5){
    size_t st=0;while(st+4<len&&!(rx[st]==0x55&&rx[st+1]==0xAA&&rx[st+2]==0xEB&&rx[st+3]==0x90))st++;
    if(st){memmove(rx,rx+st,len-st);len-=st;if(len<5)break;}
    size_t next=0;for(size_t i=4;i+4<len;i++)if(rx[i]==0x55&&rx[i+1]==0xAA&&rx[i+2]==0xEB&&rx[i+3]==0x90){next=i;break;}
    if(next){if(protocol_.parseFrame(rx,next,g_bmsData))g_bmsData.online=true;memmove(rx,rx+next,len-next);len-=next;continue;}
    if(len>=300&&protocol_.parseFrame(rx,300,g_bmsData)){g_bmsData.online=true;memmove(rx,rx+300,len-300);len-=300;continue;}
    break;
  }
}
void JkBle::request(uint8_t cmd){if(!ch_)return;uint8_t f[20];protocol_.buildCommand(cmd,counter_++,f);ch_->writeValue(f,20,false);}
void JkBle::loop(){if(!connected())return;if(millis()-lastRequest_>5000){request(0x96);request(0x97);lastRequest_=millis();}}
bool JkBle::connected()const{return client_&&client_->isConnected();}
