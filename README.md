# LoRe (*Luminescent Ocular Robotic Engine*)

[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](scripts/build.sh)
[![Tests](https://img.shields.io/badge/Tests-9%20Passed-brightgreen.svg)](scripts/run_tests.sh)
[![Platform](https://img.shields.io/badge/Hardware-ESP32--S3%20SuperMini-blue.svg)](https://www.espressif.com/)
[![Display](https://img.shields.io/badge/Display-SSD1306%20OLED%20(128x64)-blueviolet.svg)](https://github.com/lovyan03/LovyanGFX)
[![Memory](https://img.shields.io/badge/Memory-Internal%20SRAM%20Only-success.svg)](docs/adr/ADR-003-sram-only-memory-budget.md)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## 1. Description

> **Autonomous biomechanical ocular synthesis and affective psychology engine for headless robotic companions on the ESP32-S3.**

**LoRe** is an asynchronous embedded robotics companion firmware that drives a 0.96" I2C OLED display (SSD1306) at a deterministic **60 FPS**. Engineered specifically for miniature form-factor microcontrollers without camera optics or external PSRAM, LoRe generates fluid, biologically plausible gaze kinetics using Flash & Hogan 5th-order minimum-jerk saccades, 2D Russell Circumplex emotional Langevin diffusion, 5 homeostatic drives, episodic vector memory, ambient information glance HUDs, and dual-band communication (Wi-Fi STA/AP and BLE NUS).

---

## 2. Demo / Preview

```
  +-------------------------------------------------------------------+
  |                      LoRe 128x64 OLED HUD                         |
  |                                                                   |
  |         (•   •)                     (•   •)                       |
  |         |     |                     |     |                       |
  |         +-----+                     +-----+                       |
  |                                                                   |
  |            \_/                         ___                        |
  |        [Joy Mood]                 [Deadpan Mood]                  |
  |                                                                   |
  |  +---------------------+   +---------------------+   +---------+  |
  |  |    14:28            |   | Jakarta  *  28.5 C  |   | [WA] Ali|  |
  |  |    Tue 8 Sep 2026   |   | Mainly Clear  *  70%|   | Meet at |  |
  |  +---------------------+   +---------------------+   +---------+  |
  |      Ambient Clock               Live Weather          Notif HUD  |
  +-------------------------------------------------------------------+
```

---

## 3. Key Features

- **60 FPS Biomechanical Kinematics**: Flash & Hogan 5th-order minimum-jerk saccade trajectory generation coupled with an underdamped second-order ocular mass-spring-damper system ($\omega_n = 32.0\text{ rad/s}, \zeta = 0.72$).
- **Rigid 2D Facial Rig (8 Expressions)**: Volume-conserving squash-and-stretch rendering for `IDLE`, `JOY`, `ANGRY`, `SMIRK`, `SHOCK`, `OVERLOAD`, `SAD`, and `DEADPAN`.
- **Russell Circumplex Affective Engine**: Stochastic Langevin diffusion across valence-arousal emotional state space within a stabilizing quartic potential well.
- **On-Device TinyML Micro-Brain**: 5 homeostatic drives (*Curiosity*, *Social*, *Boredom*, *Fatigue*, *Mischief*) coupled to Markov action selection and circadian energy cycles.
- **Episodic Latent Vector Memory**: 32-entry episodic memory bank with cosine similarity resonance and NVS flash persistence.
- **Hybrid Ambient Utility Glances**: Living mini-ocular companion header paired with Digital Clock, Open-Meteo Weather forecast with alternating astronomical dawn/dusk indicator (`^05:49 v17:51`), Ntfy.sh Phone Notifications with physical startle reaction, and Turn-by-Turn Navigation HUD.
- **Real Astronomical Sunrise & Sunset Sync**: Daily astronomical ephemeris queried from Open-Meteo dynamically anchors dusk wind-down, morning awakening sequence, and ambient display dimming to actual local solar conditions.
- **OLED Deep Sleep at Night (Anti-Burn-In)**: Automatically powers down the SSD1306 OLED panel (`lcd.sleep()`, 0xAE display off) between 01:00 and 05:30/sunrise, eliminating pixel burn-in and dark room glare. Instantly wakes with physical startle reaction for notifications or touch interactions.
- **Weather-Aware Affective Kinetics**: Ambient rain/heat responsive Langevin emotional modulation and contemplative upward skyward gaze saccades during rainfall.
- **Responsive Bento Grid Web UI**: Embedded Single Page Application on Port 80 for remote gaze steering, live telemetry visualization, expression control, and Web OTA updates.
- **Dual-Band Connectivity**: BLE Nordic UART Service (NUS) for companion phone synchronization and Wi-Fi STA with automatic SoftAP captive portal fallback (192.168.18.16).
- **Internal SRAM Footprint**: Operates entirely within ~56.9 KB of dynamic memory (17% of internal SRAM) with PSRAM completely disabled.

---

## 4. Tech Stack & Architecture

### Dual-Core FreeRTOS Partitioning

```
+---------------------------------------------------------------------------------+
| CORE 0: Background Services, Gaze Coordination & Power Scaling (Priority 2)    |
| - Gaze Coordination Task (gazeTask @ 100 ms period)                             |
| - Virtual Target Expiry & External Stimulus Management (setVirtualTarget)       |
| - Dynamic Frequency Scaling (DFS: 240 MHz Active <-> 80 MHz Sleep Standby)      |
| - Asynchronous Wi-Fi Station & AP Fallback with Captive Portal                  |
| - Embedded HTTP Web Control Server (Port 80)                                    |
| - BLE Nordic UART Service (NUS) Telemetry & Companion Synchronization          |
| - Periodic Background Weather Client (Open-Meteo REST API)                      |
| - Cloud Push Notification Client (Ntfy.sh background stream)                   |
+---------------------------------------------------------------------------------+
                                         |
                         [ Spinlock: g_target_mutex ]
                         [ Spinlock: g_weather_mutex ]
                         [ Spinlock: g_notification_mutex ]
                         [ Spinlock: g_nav_mutex ]
                                         |
                                         v
+---------------------------------------------------------------------------------+
| CORE 1: oledTask (Priority 1, 60 FPS Kinematics & Rendering Loop)               |
| - 2D Russell Circumplex Affective Engine (Valence-Arousal Langevin Diffusion)   |
| - On-Device TinyML Micro-Brain (Homeostatic Drives & Markov Action Selection)   |
| - Unified Rigid 2D Facial Rig (8 Expressions, Eyes, Eyebrows, Mouth)           |
| - Coordinate Hysteresis Filtering (getFilteredOx, getFilteredOy)                |
| - Ocular Dynamics (32.0 rad/s Underdamped Mass-Spring-Damper, zeta = 0.72)      |
| - 5th-Order Minimum-Jerk Saccades (Flash & Hogan Formulation)                   |
| - Fixation Micro-Kinetics (Mean-Reverting Brownian Random Walk)                 |
| - Non-Blocking Eyelid State Machine (Idle -> Closing -> Opening -> Blink-Chain) |
| - OLED Anti-Burn-In Protection (+/-1 px Micro-Shift during Standby)             |
| - Ambient Screens (Clock, Open-Meteo Weather, Push Notification, Turn-by-Turn)   |
| - LovyanGFX 1-Bit Monochrome Sprite Renderer (1.0 MHz Fast-Mode Plus I2C Bus)   |
+---------------------------------------------------------------------------------+
```

### Module Structure

| Layer | Path | Responsibility |
| :--- | :--- | :--- |
| **Config** | `src/config/device_config.h` | NVS persistent storage (`lore_cfg`), credentials, settings |
| **Config** | `src/config/lore_config.h` | Hardware pinouts, timing constants, FreeRTOS budgets |
| **Types** | `src/types/lore_types.h` | Cross-module structs, enums, emotion vectors, telemetry models |
| **Math** | `src/math/kalman_filter.h` | 1D discrete Kalman filter with covariance clamping |
| **Math** | `src/math/kinematics.h` | 5th-order minimum-jerk splines & mass-spring-damper ODEs |
| **Math** | `src/math/affective_engine.h`| Russell Circumplex Langevin stochastic diffusion solver |
| **AI** | `src/ai/brain_engine.h` | 5 homeostatic drives and feedforward neural action selector |
| **AI** | `src/ai/personality_engine.h`| Four-factor OCEAN traits and sinusoidal circadian cycle |
| **AI** | `src/ai/memory_engine.h` | 32-slot episodic vector memory with cosine similarity |
| **AI** | `src/ai/markov_corpus.h` | Markov chain thought generator |
| **Core** | `src/core/display_engine.h` | LovyanGFX I2C bus timing, `oledTask` loop, auto-brightness |
| **Core** | `src/core/facial_renderer.h`| 2D facial rig geometry (eyes, mouth, blush) and easing curves |
| **Core** | `src/core/ambient_screens.h`| Clock, Weather, Notification, and Navigation screen drawing |
| **Core** | `src/core/gaze_engine.h` | Virtual gaze coordination, stimulus decay & DFS power scaling |
| **Net** | `src/net/wifi_manager.h` | Wi-Fi STA, SoftAP fallback, mDNS, NetBIOS, DNS captive portal |
| **Net** | `src/net/http_server.h` | Port 80 HTTP server, REST control, Web OTA update |
| **Net** | `src/net/ble_manager.h` | BLE GATT Nordic UART Service (NUS) server |
| **Net** | `src/net/telemetry_service.h`| Canonical telemetry snapshot JSON serializer |
| **Net** | `src/net/weather_client.h` | Background Open-Meteo REST weather fetcher |
| **Net** | `src/net/notification_client.h`| Background Ntfy.sh stream client |
| **Net** | `src/net/net_utils.h` | Shared JSON string extraction & app name classifier |

---

## 5. Prerequisites

- **Microcontroller**: ESP32-S3 SuperMini (Xtensa Dual-Core LX7, 4MB Flash, No PSRAM).
- **Display**: 0.96" Monochrome I2C OLED (SSD1306, 128x64 pixels).
- **Tooling**:
  - `arduino-cli` $\ge$ 1.0.0
  - `esp32:esp32` core $\ge$ 3.3.0
  - `LovyanGFX` library $\ge$ 1.2.0
  - Host C++20 compiler (`g++` or `clang++`) for unit testing

---

## 6. Installation & Setup

### Hardware Wiring (I2C Bus)

| ESP32-S3 SuperMini Pin | OLED SSD1306 Pin | Function |
| :--- | :--- | :--- |
| **3V3** | VCC | 3.3V Power Supply |
| **GND** | GND | Common Ground |
| **GPIO 3** | SCL | I2C Clock (1.0 MHz Fast-Mode Plus) |
| **GPIO 12** | SDA | I2C Data |

### Firmware Compilation & Flashing

1. Clone repository:
   ```bash
   git clone https://github.com/fioren/LoRe.git
   cd LoRe
   ```

2. Compile firmware using the project build script:
   ```bash
   bash scripts/build.sh
   ```

3. Flash via `arduino-cli`:
   ```bash
   arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:esp32s3:PSRAM=disabled,PartitionScheme=min_spiffs .
   ```

---

## 7. Configuration

Device configuration can be modified in `src/config/lore_config.h` or dynamically configured at runtime via the Web Dashboard and stored in non-volatile storage (NVS `lore_cfg` namespace):

```cpp
/* Wi-Fi & Network Defaults */
#define WIFI_STA_DEFAULT_SSID       "YourWiFiSSID"
#define WIFI_STA_DEFAULT_PASS       "YourWiFiPassword"
#define WIFI_FALLBACK_AP_SSID       "LoRe-Setup"
#define WIFI_FALLBACK_AP_PASS       "12345678"

/* Bluetooth Low Energy */
#define BLE_DEVICE_NAME_DEFAULT     "LoRe-Sense"

/* Open-Meteo Weather Coordinates */
#define WEATHER_DEFAULT_CITY        "Jakarta"
#define WEATHER_DEFAULT_LAT         -6.2088f
#define WEATHER_DEFAULT_LON         106.8456f
```

---

## 8. Usage / Quick Start

1. **Power On**: Upon boot, the OLED displays initialization status while FreeRTOS tasks spawn.
2. **Initial Network Setup**:
   - If configured Wi-Fi is unavailable, LoRe starts SoftAP mode (`SSID: LoRe-Setup`, `Password: 12345678`).
   - Connect mobile device or PC to `LoRe-Setup` and navigate to `http://192.168.18.16` or `http://lore.local`.
   - Enter your local Wi-Fi credentials in the setup portal.
3. **Web Dashboard**: Once connected to Wi-Fi, open `http://<device-ip>/` to access the Bento Grid control center.
4. **BLE Mobile Companion**: Pair with Bluetooth device `LoRe-Sense` to stream real-time telemetry or push notifications.

---

## 9. API Reference

### HTTP REST Endpoints (Port 80)

| Endpoint | Method | Payload / Format | Description |
| :--- | :--- | :--- | :--- |
| `/` | `GET` | HTML | Bento Grid Control Dashboard |
| `/telemetry` | `GET` | JSON | Full telemetry snapshot (gaze, affect, drives, memory) |
| `/set_expression` | `POST` | `{"expr": 0..7}` | Override facial expression or `-1` for auto |
| `/set_gaze` | `POST` | `{"x": -1..1, "y": -1..1}` | Direct gaze to normalized coordinate |
| `/set_brightness` | `POST` | `{"brightness": 0..255}` | Adjust SSD1306 contrast brightness |
| `/set_auto_brightness`| `POST` | `{"enabled": true}` | Toggle dynamic ambient auto-brightness |
| `/api/notify` | `POST` | `{"app":"WA","message":"Hi"}`| Dispatch notification banner to OLED |
| `/trigger_weather` | `POST` | - | Force immediate weather glance popup |
| `/trigger_clock` | `POST` | - | Force immediate digital clock popup |
| `/update` | `POST` | Binary (firmware.bin) | Web OTA firmware flash |

### BLE Nordic UART Service (NUS)

- **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX Characteristic (Write)**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
  - Accepts raw text commands (e.g. `EXPR:1`, `BRIGHTNESS:200`) or JSON packets (`{"cmd":"nav","icon":"turn_right","dist":"200m"}`).
- **TX Characteristic (Notify)**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
  - Emits command responses and periodic 120-byte chunked telemetry streams.

---

## 10. Testing

LoRe includes an automated host unit test suite executed without hardware dependencies. The suite verifies algorithmic convergence, numerical stability, and memory bounds using `g++` (`-std=c++20`):

```bash
bash scripts/run_tests.sh
```

### Test Coverage

- `test_minimum_jerk`: Validates boundary velocities, accelerations, and jerk continuity.
- `test_kalman_convergence`: Tests innovation update and covariance convergence under Gaussian noise.
- `test_affective_langevin`: Verifies Russell Circumplex mean-reversion and bounded state containment.
- `test_personality_circadian`: Asserts diurnal circadian oscillation bounds and trait modulation.
- `test_episodic_memory`: Validates 32-slot ring buffer eviction and cosine resonance ranking.
- `test_brain_inference`: Tests feedforward activation and homeostatic drive saturation.
- `test_kinematics_feedforward`: Asserts mass-spring-damper settling time and damping ratio bounds.
- `test_ble_telemetry`: Verifies JSON telemetry buffer formatting and schema compliance.
- `test_notification_parser`: Validates JSON string extraction, message wrapping, and app identification.

---

## 11. Deployment

### Production Compilation Checklist

1. Confirm `PartitionScheme=min_spiffs` to maximize application code space (up to ~1.9 MB flash partition).
2. Confirm `PSRAM=disabled` in build flags.
3. Validate dynamic memory allocation remains $< 80\text{ KB}$ post-compile:
   ```bash
   bash scripts/build.sh
   ```

### Over-The-Air (OTA) Updating

1. Generate binary: `arduino-cli compile --export-binaries ...`
2. Navigate to `http://<lore-ip>/` in your browser.
3. Upload `build/esp32.esp32.esp32s3/LoRe.ino.bin` under the **Firmware OTA** tile.
4. Device flashes and restarts automatically within 1.5 seconds.

---

## 12. Roadmap

- [x] Host C++20 unit test harness with zero Arduino mock dependencies.
- [x] Decouple God files into modular deep modules (`facial_renderer`, `ambient_screens`, `device_config`, `telemetry_service`, `net_utils`).
- [ ] MPU6050 / QMI8658 I2C inertial measurement unit (IMU) integration for head-tilt compensation.
- [ ] Capacitive touch pad integration for tactile petting and affection reinforcement.
- [ ] Expanded BLE peripheral protocol support for smartwatch companion apps.

---

## 13. Contributing

1. Review the architecture guidelines in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) and accepted decisions in [docs/adr/](docs/adr/).
2. Adhere to deep module principles: keep interfaces narrow and encapsulate internal state.
3. Strictly maintain the zero-PSRAM internal SRAM budget (< 80 KB dynamic RAM).
4. Run `bash scripts/run_tests.sh` to ensure zero test regressions before submitting a pull request.

---

## 14. License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## 15. Acknowledgements

- **LovyanGFX**: High-performance, low-latency graphics driver by Lovyan03.
- **Espressif Systems**: ESP-IDF FreeRTOS dual-core framework.
- **Flash & Hogan**: Theoretical foundation for minimum-jerk biological movement models.
- **Open-Meteo**: Free weather forecast API without proprietary API keys.
- **Ntfy.sh**: Real-time push notification streaming service.
