# LoRe Telemetry and Control API Specification

## 1. Overview

LoRe hosts a lightweight, asynchronous single-port HTTP server on the ESP32-S3:
- **Port 80:** Web Control Dashboard, Wi-Fi Configuration API, System Status, Ambient HUD Triggers, and JSON Telemetry.

---

## 2. API Endpoints

### 2.1 Web Dashboard
- **URL:** `GET http://<ESP32_IP>/`
- **Response:** `text/html` (Embedded Bento Grid production dashboard with dark matte technical surface).

### 2.2 Real-Time JSON Telemetry
- **URL:** `GET http://<ESP32_IP>/telemetry`
- **Response:** `application/json`
- **Schema:**
```json
{
  "type": "telemetry",
  "detected": true,
  "x": 210,
  "y": 140,
  "w": 180,
  "h": 220,
  "cx": 300,
  "cy": 250,
  "err_x": -6.2,
  "err_y": 4.1,
  "conf": 0.94,
  "human_likelihood": 0.85,
  "fps_ai": 60.0,
  "fw": 640,
  "fh": 480,
  "vx": -1.2,
  "vy": 0.4,
  "prox": 0.70,
  "num_cands": 0,
  "insp_idx": 0,
  "c0_cx": 0,
  "c0_cy": 0,
  "c0_w": 0,
  "c0_h": 0,
  "c0_p": 0.0,
  "c1_cx": 0,
  "c1_cy": 0,
  "c1_w": 0,
  "c1_h": 0,
  "c1_p": 0.0,
  "c2_cx": 0,
  "c2_cy": 0,
  "c2_w": 0,
  "c2_h": 0,
  "c2_p": 0.0,
  "expr": 0,
  "expr_name": "IDLE",
  "is_manual": false,
  "valence": 0.15,
  "arousal": 0.45,
  "curiosity": 0.60,
  "social": 0.50,
  "boredom": 0.20,
  "fatigue": 0.10,
  "mischief": 0.30,
  "thought": "observing quiet horizon",
  "interact_s": 120,
  "solitude_s": 45,
  "bonding": 0.42,
  "life_s": 3600,
  "mem_count": 8,
  "mem_res": 0.88,
  "mem_expr": 0,
  "heap_free": 284000,
  "psram_free": 0,
  "uptime_s": 3600,
  "cpu_mhz": 240,
  "cam_sleep": false,
  "cam_online": false,
  "brightness": 128,
  "auto_brightness": true,
  "personality": {
    "boldness": 0.65,
    "volatility": 0.40,
    "playfulness": 0.70,
    "attachment": 0.55
  },
  "circadian": {
    "energy": 0.82,
    "mood_offset": 0.05,
    "phase_pct": 42.5
  }
}
```

### 2.3 Expression Control
- **`POST /set_expression`:** Sets the manual face expression override or restores default auto mood engine.
  - **Body:** `{"expr": 0}` (0: IDLE, 1: HAPPY) or `{"expr": "auto"}` / `{"expr": -1}` to restore default.
  - **Response:** `{"status": "ok"}`

### 2.4 Ambient HUD & Screen Triggers
- **`POST /trigger_weather`:** Triggers 6-second weather forecast glance HUD screen display on the OLED panel.
  - **Response:** `{"status": "ok"}`
- **`POST /trigger_clock`:** Triggers 6-second digital clock and date glance HUD screen display on the OLED panel.
  - **Response:** `{"status": "ok"}`
- **`POST /sync_time`:** Synchronizes real-time epoch from client browser to internal RTC.
  - **Body:** `{"epoch": 1725810000}`
  - **Response:** `{"status": "ok"}`
- **`POST /api/notify`:** Dispatches notification banner popup to OLED panel with startle reaction.
  - **Body:** `{"app": "WA", "title": "John", "message": "Arrived at station"}`
  - **Response:** `{"status": "ok"}`
- **`GET /api/ntfy`:** Retrieves current Ntfy.sh background stream topic and enabled status.
  - **Response:** `{"topic": "lore-demo", "enabled": true}`
- **`POST /api/ntfy`:** Configures Ntfy.sh background stream topic and enabled flag with NVS persistence.
  - **Body:** `{"topic": "my-ntfy-topic", "enabled": true}`
  - **Response:** `{"status": "ok"}`

### 2.5 Wi-Fi Configuration
- **`GET /get_wifi`:** Returns current saved credentials and mode flag.
- **`POST /save_wifi`:** Updates SSID and password in NVS and reboots device.
- **`POST /switch_mode`:** Toggles between AP and STA mode in NVS and reboots device.

### 2.6 WiFi Network Scanner
- **URL:** `GET http://<ESP32_IP>/scan_wifi`
- **Response:** `application/json`
- **Schema:**
```json
{
  "networks": [
    {"ssid": "MyNetwork", "rssi": -45, "enc": "WPA2"},
    {"ssid": "OpenNet", "rssi": -72, "enc": "OPEN"}
  ],
  "count": 2
}
```

### 2.7 System Information
- **URL:** `GET http://<ESP32_IP>/system_info`
- **Response:** `application/json`
- **Schema:**
```json
{
  "firmware": "2.5.0",
  "compiled": "Aug 23 2026 12:00:00",
  "chip": "ESP32-S3",
  "cores": 2,
  "cpu_mhz": 240,
  "heap_free": 125432,
  "heap_min": 98000,
  "psram_free": 7340032,
  "psram_total": 8388608,
  "uptime_s": 3600,
  "wifi_rssi": -52,
  "wifi_mode": "STA",
  "ip": "192.168.1.100",
  "brightness": 128,
  "auto_brightness": true
}
```

### 2.8 Autonomous Gaze Control
- **`POST /set_gaze`:** Directs LoRe's attention and eye gaze toward normalized virtual coordinates in real time.
  - **Body:** `{"x": 0.35, "y": -0.20, "duration_ms": 3500}`
  - **Parameters:**
    - `x` (float): Horizontal normalized gaze position [-1.0 (left) .. +1.0 (right)]
    - `y` (float): Vertical normalized gaze position [-1.0 (up) .. +1.0 (down)]
    - `duration_ms` (float, optional): Attention hold duration in milliseconds (default: 3000 ms)
  - **Response:** `{"status": "ok"}`

### 2.9 Web Over-The-Air (OTA) Update
- **`POST /update`:** Uploads binary firmware (`.bin`) directly to ESP32-S3 flash partition.
  - **Payload:** Raw binary firmware bytes (`application/octet-stream`)
  - **Response:** `{"status": "ok", "message": "Firmware flashed! Rebooting..."}`

### 2.10 Display Brightness Control
- **`POST /set_brightness`:** Adjusts OLED display brightness in real-time ($0 - 255$) with optional NVS persistence.
  - **Body:** `{"brightness": 180, "save": true}`
  - **Response:** `{"status": "ok"}`

### 2.11 Weather Location & Configuration
- **`POST /set_weather`:** Configures city name, geographic coordinates, and automatic standby popup flag.
  - **Body:** `{"city": "Jakarta", "lat": -6.2088, "lon": 106.8456, "enabled": true}`
  - **Response:** `{"status": "ok"}`
- **`GET /weather_info`:** Retrieves latest parsed Open-Meteo observation payload.
  - **Response:** `application/json`
  - **Schema:**
```json
{
  "city": "Jakarta",
  "lat": -6.2088,
  "lon": 106.8456,
  "enabled": true,
  "valid": true,
  "temp": 29.5,
  "humidity": 78,
  "code": 3,
  "condition": "OVERCAST",
  "last_sync_s": 120
}
```
- **`POST /trigger_weather`:** Triggers instantaneous 6-second weather screen display on the OLED panel.
  - **Response:** `{"status": "ok"}`

### 2.12 Extended Telemetry Fields
The `/telemetry` and BLE telemetry payloads include dynamic system, affective, and camera state metrics:
- `valence` (float): Current emotional valence [-1.0, 1.0]
- `arousal` (float): Current emotional arousal [0.0, 1.0]
- `curiosity`, `social`, `boredom`, `fatigue`, `mischief` (float): Homeostatic drive states [0.0, 1.0]
- `thought` (string): Real-time cognitive inner thought summary
- `bonding` (float): Bonding level with human companion [0.0, 1.0]
- `cam_sleep` (bool): Low-power standby sleep status flag (legacy companion schema compatibility)
- `cam_online` (bool): Legacy hardware status flag (always false in camera-less LoRe architecture)
- `heap_free` (uint): Free internal heap in bytes
- `psram_free` (uint): Free PSRAM in bytes
- `uptime_s` (uint): System uptime in seconds
- `cpu_mhz` (int): Current CPU frequency in MHz

---

## 3. Bluetooth Low Energy (BLE) NUS Telemetry Protocol

LoRe exposes a high-throughput Nordic UART Service (NUS) over BLE GATT for mobile applications:
- **Service UUID:** `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX Characteristic (Write / Write Without Response):** `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **TX Characteristic (Notify):** `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`

### 3.1 On-Demand Telemetry Snapshot
- **Command:** `{"cmd": "get_telemetry"}` or `{"cmd": "telemetry"}` or raw string `TELEMETRY`
- **Behavior:** Returns the full JSON telemetry payload via TX notification chunked into safe MTU packets without waking the robot from low-power standby sleep.

### 3.2 Continuous Telemetry Streaming
- **Start Streaming:** `{"cmd": "stream_telemetry", "enable": true, "interval": 500}` or `STREAM_TELEMETRY:500`
- **Stop Streaming:** `{"cmd": "stream_telemetry", "enable": false}` or `STREAM_TELEMETRY:0`
- **Behavior:** LoRe's background FreeRTOS BLE telemetry task automatically transmits real-time telemetry updates at the specified period (e.g. 500ms) over GATT notifications.

### 3.3 Low-Power Standby Architecture
Querying or streaming telemetry via BLE or HTTP `/telemetry` operates strictly passively and **never forces the cognitive engine to wake from low-power standby sleep**. The system enters standby sleep (`STATE_SLEEP_RECON`) automatically when not actively receiving stimulus or interacting, preserving battery and reducing thermal dissipation.


