# V3.5 FlickerFix + Vue + LittleFS + WebSocket

## 1. 最终结构

ESP32 同时运行：

- ANT BMS BLE：连接并解析 BMS
- TFT 320x170：保留原 V3.5 本地界面
- Wi-Fi AP：SSID `BMS-DASH`
- WebServer：HTTP 80，提供 Vue 静态文件
- WebSocket：端口 81，实时推送 BMS JSON
- LittleFS：保存 Vue 编译后的网页文件

网页数据链路：

`ANT BMS -> ESP32 BmsData -> buildStatusJson() -> WebSocket -> Vue`

网页操作链路：

`Vue -> WebSocket -> processPhoneCommand() -> 原 V3.5 功能`

保留的命令：

- `PING`
- `GET_STATUS`
- `SCAN_BMS`
- `SELECT_BMS:<MAC>`
- `FORGET_BMS`
- `RECONNECT_BMS`
- `REBOOT`

## 2. Vue 编译

进入仓库根目录：

```text
web-vue
```

执行：

```bash
npm install
npm run build
```

生成：

```text
web-vue/dist/
├── index.html
└── assets/
    ├── *.js
    └── *.css
```

Vite 使用 `base: './'`，因此网页放入 ESP32 LittleFS 后可以直接从 `http://192.168.4.1/` 工作。

## 3. 上传到 ESP32 LittleFS

把 Vue 的 `dist/` **里面的内容**复制到：

```text
esp32/ANT_BMS_TFT_Dashboard_V3_5_FlickerFix/data/
├── index.html
└── assets/
    ├── *.js
    └── *.css
```

注意不是：

```text
data/dist/index.html
```

而是：

```text
data/index.html
```

### Arduino IDE 2.x 推荐方法

本项目 firmware 已使用 Arduino-ESP32 的 `LittleFS` API。

Arduino IDE 2.x 可以安装 LittleFS uploader，然后使用：

```text
Command Palette
    -> Upload LittleFS to Pico/ESP8266/ESP32
```

或者使用支持 ESP32 的 LittleFS sidebar uploader。

上传的是当前 sketch 的 `data/` 文件夹，不是普通的代码上传。

官方/社区工具说明：

- Earle Philhower LittleFS uploader：兼容 Arduino IDE 2.2.1+
- Arduino IDE 中编译一次 firmware 后，再执行 LittleFS upload
- 选择的 partition scheme 必须包含 filesystem data partition

## 4. ESP32 端

启动时会：

1. 挂载 LittleFS
2. 如果挂载失败，尝试格式化后重新挂载
3. 启动 Wi-Fi AP
4. 启动 HTTP 80
5. 启动 WebSocket 81

HTTP：

```text
http://192.168.4.1/
```

状态：

```text
http://192.168.4.1/status
```

LittleFS 状态：

```text
http://192.168.4.1/fs-info
```

WebSocket：

```text
ws://192.168.4.1:81/
```

## 5. 网页实时数据

ESP32 的 `sendSetupJson()` 同时兼容：

- 原来的 BLE 手机端
- 新的 WebSocket Vue 网页

所以 BMS 状态、扫描结果、错误、选择结果等消息可以同时广播给 Web UI。

状态 JSON 包含：

- voltage
- current
- soc
- power
- mos_temperature
- total_capacity
- remaining_capacity
- delta_cell_mv
- max_cell_voltage
- min_cell_voltage
- rssi
- selected_bms
- name
- state
- firmware

## 6. 每次更新网页的实际流程

以后修改网页，不需要重新修改 ESP32 C++。

只需要：

```bash
cd web-vue
npm run build
```

然后把：

```text
web-vue/dist/*
```

复制到：

```text
esp32/ANT_BMS_TFT_Dashboard_V3_5_FlickerFix/data/
```

再执行 LittleFS Upload。

通常无需重新烧录 firmware。

## 7. 注意

如果重新烧录 firmware 时选择了会擦除/重建 filesystem 的方式，网页文件可能需要再次上传。

V3.5 的固件和网页现在是解耦的：

```text
firmware
  └── BMS / BLE / TFT / WebServer / WebSocket

LittleFS
  └── Vue index.html + assets

Vue
  └── 页面/UI/交互
```
