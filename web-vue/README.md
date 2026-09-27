# BMS-DASH Vue Web UI

Vue 3 + Vite frontend for ESP32 V3.5 FlickerFix.

## Development

```bash
npm install
npm run dev
```

The ESP32 WebSocket endpoint is:

```text
ws://<ESP32-IP>:81/
```

For the final ESP32 deployment:

```bash
npm run build
```

Then copy the **contents** of `dist/` into:

```text
esp32/ANT_BMS_TFT_Dashboard_V3_5_FlickerFix/data/
```

The Vite configuration uses relative asset URLs (`base: './'`), so the same build can be served from ESP32 LittleFS.

## Supported ESP32 commands

- PING
- GET_STATUS
- SCAN_BMS
- SELECT_BMS:<MAC>
- FORGET_BMS
- RECONNECT_BMS
- REBOOT

## Runtime

The production page automatically connects to:

```text
ws://window.location.hostname:81/
```

Therefore the same Vue build works when opened from the ESP32 AP at:

```text
http://192.168.4.1/
```
