# JK_BMS_ESP32C3_ST7789_V1

ESP32-C3 + JK BMS BLE + ST7789 1.9" (320×170) 电池仪表项目。

当前 main 主线以 JK_BMS_ESP32C3_ST7789_V1 为主项目，工程位于：

esp32/JK_BMS_ESP32C3_ST7789_V1/

## 当前版本
- ESP32-C3
- ST7789 1.9" 320×170
- JK BLE / JK02_24S / JK02_32S
- 局部刷新 UI
- Web 配置与 BLE 扫描
- 剩余里程计算
- 开机自动扫描并连接

## 本次修复
### ST7789 开机显示
开机初始化阶段先关闭 GPIO5 背光：
1. GPIO5 设置为 OUTPUT。
2. GPIO5 拉 LOW，保持背光关闭。
3. 延时 20ms 等待电源稳定。
4. 初始化 ST7789。
5. 清空屏幕。
6. 等待 30ms。
7. GPIO5 拉 HIGH 开启背光。

这样可以避免 ST7789 初始化期间背光提前点亮造成的闪烁、白屏或黑屏。

### 背光闪烁
修复了原先 setup() 中先 delay(500)、而 GPIO5 尚未初始化的问题，同时取消在 ST7789 初始化之前开启背光的做法。

## 工程目录

esp32/
└── JK_BMS_ESP32C3_ST7789_V1/
    ├── platformio.ini
    ├── tft_setup.h
    ├── JkConfig.h
    └── src/
        ├── main.cpp
        ├── BmsData.h/.cpp
        ├── JkProtocol.h/.cpp
        ├── JkBle.h/.cpp
        ├── Display.h/.cpp
        ├── FontGB2312.h/.cpp
        ├── Preferences.h/.cpp
        └── WebConfig.h/.cpp