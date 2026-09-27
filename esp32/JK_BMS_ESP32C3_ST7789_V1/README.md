# ESP32-C3 + JK BMS BLE + ST7789 1.9"

屏幕接线：
BL=GPIO5
CS=GPIO3
DC=GPIO2
RES=GPIO10
SDA/MOSI=GPIO7
SCL/SCK=GPIO6

本工程使用 NimBLE-Arduino + TFT_eSPI。
JK BLE 常用 Service FFE0、Characteristic FFE1；连接后订阅 FFE1，并发送 0x96 与 0x97 请求，之后接收 0x02 主数据帧。

数据解析参考 syssi/esphome-jk-bms 的公开 JK BLE 协议实现。该实现记录了 0x02 主帧中的单体电压、总电压、电流、温度等字段，并说明 JK02_24S / JK02_32S 的差异。
