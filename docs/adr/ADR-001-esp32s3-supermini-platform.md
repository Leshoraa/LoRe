# ADR-001: Target Platform Selection - ESP32-S3 SuperMini

## Status
Accepted

## Context
The previous robotic firmware (KoRe) targeted the Seeed Studio XIAO ESP32-S3 Sense board, which integrates an on-board camera module (OV2640/OV3660) and 8MB of external Octal PSRAM. 

For the LoRe project, the mechanical and physical constraints require a compact, low-cost microcontroller without camera optics, utilizing a standard 0.96-inch monochrome I2C OLED display (SSD1306). The ESP32-S3 SuperMini board was selected as the target platform.

The ESP32-S3 SuperMini features:
- Dual-Core 32-bit Xtensa LX7 processor running at up to 240 MHz.
- 4MB embedded SPI flash memory.
- No external PSRAM (PSRAM Disabled in firmware settings).
- Compact ultra-miniature pinout with directly accessible GPIO pins (e.g. GPIO 5 for SCL, GPIO 6 for SDA).

## Decision
We adopt the ESP32-S3 SuperMini as the primary hardware platform for LoRe:
1. Pin I2C display communication to GPIO 5 (SCL) and GPIO 6 (SDA) on hardware I2C port 0.
2. Run FreeRTOS dual-core task partitioning: Core 0 handles network stacks, HTTP server, BLE companion, and power scaling; Core 1 handles 60 FPS ocular kinematics and sprite rendering.
3. Configure the Arduino flash partition scheme to `min_spiffs` (1.9MB application space / 128KB SPIFFS) to accommodate the Wi-Fi, BLE, and LovyanGFX stacks within 4MB total flash.

## Alternatives Considered
- **ESP32-C3 SuperMini:** Single-core RISC-V @ 160 MHz. Rejected because lack of a second core prevents dedicating a core to zero-jitter 60 FPS display rendering while simultaneously servicing Wi-Fi and BLE interrupts.
- **ESP8266:** Single-core 80/160 MHz. Rejected due to insufficient internal SRAM, lack of BLE, and single-core limitations.
- **Standard ESP32-WROOM:** Dual-core Xtensa LX6. Rejected due to larger form factor and lack of native USB-JTAG/CDC.

## Consequences
- Requires strict internal SRAM budgeting (< 380 KB usable heap) with zero reliance on external PSRAM.
- Dedicated hardware I2C Fast-Mode Plus overclock (1.0 MHz) achieves smooth 60 FPS rendering on Core 1 without bus contention.
