# LoRe System Architecture and Core Partitioning

## 1. Executive Summary

**LoRe** (*Luminescent Ocular Robotic Engine*) is an asynchronous, dual-core embedded robotic and biomechanical ocular synthesis system designed for the **ESP32-S3 SuperMini** (Dual-Core Xtensa LX7 @ 240 MHz).

The firmware architecture decouples network communication, HTTP dashboard services, BLE companion synchronization, and power management on **Core 0** from the real-time **60 FPS** ocular kinematics and LovyanGFX 1-bit sprite rendering engine on **Core 1**.

---

## 2. Directory Structure and Deep Module Boundaries

```
LoRe/
├── LoRe.ino                   # Master firmware entrypoint and dual-core FreeRTOS task launcher
├── include/                   # Backward-compatible public forwarding headers
│   ├── lore_config.h          # Config forwarding header
│   ├── lore_types.h           # Core types forwarding header
│   ├── lore_kalman.h          # Kalman filter forwarding header
│   ├── lore_kinematics.h      # Kinematics forwarding header
│   ├── lore_affective.h       # Affective engine forwarding header
│   ├── lore_ai.h              # Brain engine forwarding header
│   └── lore_personality.h     # Personality engine forwarding header
├── src/
│   ├── config/                # Configuration and persistent storage layer
│   │   ├── device_config.h/cpp# NVS (lore_cfg) single source of truth for Wi-Fi, BLE, OLED & weather
│   │   ├── lore_config.h      # Hardware pinouts, FreeRTOS budgets, intervals, and defaults
│   │   └── env.example.h      # Environment secrets template
│   ├── types/                 # Shared data contracts and enums
│   │   └── lore_types.h       # Target, candidate, drive, emotion, weather & notification structures
│   ├── math/                  # Numerical kinematics and affective models
│   │   ├── kalman_filter.h/cpp# 1D discrete Kalman filter with covariance clamping
│   │   ├── kinematics.h/cpp   # 5th-order minimum-jerk saccades & underdamped mass-spring-damper
│   │   └── affective_engine.h/cpp # Russell Circumplex Langevin stochastic diffusion engine
│   ├── ai/                    # Autonomous behavior and cognitive synthesis
│   │   ├── brain_engine.h/cpp # 5 homeostatic drives and feedforward neural action selector
│   │   ├── personality_engine.h/cpp # Four-factor OCEAN-derived traits and circadian rhythm
│   │   ├── memory_engine.h/cpp# 32-slot episodic vector memory with cosine similarity retrieval
│   │   └── markov_corpus.h/cpp# Markov chain thought string synthesis
│   ├── core/                  # Display orchestration and ocular rendering
│   │   ├── display_engine.h/cpp # LovyanGFX I2C bus setup, oledTask loop, auto-brightness & burn-in shift
│   │   ├── facial_renderer.h/cpp# 2D rigid geometry (eyes, mouth, blush) and expression easing
│   │   ├── ambient_screens.h/cpp# Clock, Open-Meteo Weather, Mobile Notification & Navigation HUD
│   │   └── gaze_engine.h/cpp  # Autonomous gaze coordination, stimulus decay & DFS power scaling
│   └── net/                   # Communication, protocols, and APIs
│       ├── wifi_manager.h/cpp # Wi-Fi STA connection, SoftAP fallback, mDNS, NetBIOS & captive portal
│       ├── http_server.h/cpp  # Asynchronous HTTP server (Port 80), REST endpoints & OTA updates
│       ├── web_ui.h           # Self-contained compressed single-page HTML/CSS/JS dashboard
│       ├── ble_manager.h/cpp  # BLE Nordic UART Service (NUS) GATT server & companion stream
│       ├── telemetry_service.h/cpp # Unified telemetry JSON serialization service
│       ├── weather_client.h/cpp # Background Open-Meteo REST forecast fetcher
│       ├── notification_client.h/cpp # Background Ntfy.sh NDJSON stream listener
│       └── net_utils.h/cpp    # Shared JSON key-value extraction and notification classification
├── tests/                     # Host unit test suite (9 tests, zero Arduino dependencies)
├── scripts/                   # Build and test orchestration scripts
│   ├── build.sh               # arduino-cli compile wrapper for ESP32-S3
│   └── run_tests.sh           # C++20 host test runner (g++ -O3 -std=c++20)
└── docs/                      # Technical specifications and architecture records
    ├── ARCHITECTURE.md        # System architecture and memory partitioning
    └── adr/                   # Architecture Decision Records (ADR-001 through ADR-004)
```

---

## 3. Dual-Core Task Allocation & Priority Topology

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

---

## 4. Inter-Core Concurrency & Spinlock Discipline

1. **Spinlock Hold Duration:**
   `portENTER_CRITICAL(&g_target_mutex)`, `portENTER_CRITICAL(&g_weather_mutex)`, `portENTER_CRITICAL(&g_notification_mutex)`, and `portENTER_CRITICAL(&g_nav_mutex)` strictly enclose struct copies and pointer swaps. Critical sections execute in $< 5 \ \mu\text{s}$.
2. **Lock Independence:**
   Core 0 and Core 1 never block each other during mathematical computation, network transactions, or I2C display updates.

---

## 5. Memory Layout and Hardware Partitioning

| Identifier | Location | Allocation Flag | Size | Architectural Role |
| :--- | :--- | :--- | :--- | :--- |
| `canvas` | Internal SRAM | Static / Startup Heap | 1,024 Bytes | 128x64 1-bit monochrome display frame buffer |
| `g_current_target` | Internal SRAM | Static Global | ~64 Bytes | Cross-core primary gaze target state |
| `s_episodic_memory`| Internal SRAM | Static Global | ~1.5 KB | 32-entry vector memory embedding bank |
| `s_drives` / Brain | Internal SRAM | Static Global | ~256 Bytes | 5 homeostatic drives and neural weights |
| `JSON buffers` | Internal SRAM | Stack / Temporary Heap | 512B - 1.6 KB | Scoped HTTP/BLE telemetry formatting buffers |

> [!NOTE]
> Total dynamic memory utilization is ~56.9 KB (17% of internal SRAM), leaving $> 270\text{ KB}$ of contiguous internal SRAM for local variables and task stacks. External PSRAM is completely disabled.

---

## 6. Dynamic Frequency Scaling (DFS) and Power States

- **Active Mode (Target Engaged or Web Client Active):**
  - CPU frequency: 240 MHz via `setCpuFrequencyMhz(240)`.
  - OLED refresh: 60 FPS (16.66 ms frame budget).
  - Wi-Fi and BLE active.
- **Standby Mode (Inactivity for $> 5000\text{ ms}$):**
  - CPU frequency: 80 MHz via `setCpuFrequencyMhz(80)`.
  - OLED refresh: 30 FPS (33.33 ms frame budget).
  - Anti-burn-in micro-pixel orbit shift ($\pm 1\text{ px}$) randomized every 35 seconds.
  - Spontaneous biological wake-up pulse every 40-100 seconds.
