# Changelog

All notable changes to the LoRe firmware project will be documented in this file.

## [1.0.0] - 2026-09-08

### Added
- Initial release of **LoRe** (*Luminescent Ocular Robotic Engine*).
- Hardware support for **ESP32-S3 SuperMini** (Dual-Core Xtensa LX7 @ 240 MHz, 4MB Flash, No PSRAM).
- 0.96-inch I2C OLED display (SSD1306, 128x64 pixels) via LovyanGFX running Fast-Mode Plus I2C at 1.0 MHz (SCL: GPIO 5, SDA: GPIO 6).
- Autonomous Ocular Gaze Engine (`src/core/gaze_engine.h` / `gaze_engine.cpp`) featuring 5th-order minimum-jerk saccades, sub-pixel noise gate, and organic exploratory glance patterns.
- Interactive virtual gaze steering endpoint (`POST /set_gaze`) enabling real-time remote eye direction via Web UI and BLE companion.
- Full Russell Circumplex affective emotion model with stochastic Langevin potential well diffusion and 5 homeostatic drives (curiosity, social, boredom, fatigue, mischief).
- Episodic & associative vector memory engine with cosine similarity resonance and NVS flash persistence.
- Circadian energy rhythm generator and Big-Five personality trait modulation.
- Asynchronous dual-core FreeRTOS architecture: Core 0 dedicated to Network, Web, BLE, and Power Management; Core 1 dedicated to real-time 60 FPS ocular kinematics and sprite rendering.
- Bento Grid responsive Web UI dashboard on Port 80 with live ocular tracking viewport, expression triggers, display brightness controls, weather configuration, and Web OTA firmware updater.
- Nordic UART Service (NUS) BLE companion connectivity for live telemetry streaming and turn-by-turn navigation ingestion.
- Complete host unit test suite (`scripts/run_tests.sh`) covering all 9 core mathematical and behavioral algorithms.

### Changed
- Decoupled vision pipeline from hardware camera sensors: removed physical camera driver (`esp_camera.h`), DVP bus wiring, and 64KB MJPEG video streaming buffers.
- Replaced camera MJPEG stream viewport on Web UI with interactive ocular tracking and touch/pointer gaze guiding canvas.
- Re-architected memory layout to operate strictly within internal SRAM (< 80KB dynamic memory footprint) without requiring external PSRAM.
