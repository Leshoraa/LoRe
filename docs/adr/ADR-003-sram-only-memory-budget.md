# ADR-003: Strict Internal SRAM-Only Memory Budget

## Status
Accepted

## Context
The ESP32-S3 microcontroller features 512 KB of internal SRAM (~380 KB available for heap allocation after FreeRTOS and system initialization). While some ESP32-S3 boards integrate external PSRAM via Octal SPI, the ESP32-S3 SuperMini board operates with **PSRAM Disabled**.

In KoRe, 64 KB of external PSRAM was allocated for double-buffered camera JPEG acquisition and another 64 KB was allocated for HTTP MJPEG video streaming (`ps_malloc`). Attempting to allocate external PSRAM buffers on a PSRAM-less system causes runtime allocation failures or immediate system panics.

## Decision
1. Ban all `ps_malloc()` and `MALLOC_CAP_SPIRAM` allocation requests throughout the entire codebase.
2. Eliminate large JPEG frame buffers and MJPEG video streaming buffers.
3. Keep all working structures statically sized or startup-allocated in internal SRAM (`MALLOC_CAP_INTERNAL`):
   - OLED 1-bit sprite buffer (128x64 pixels): 1,024 bytes.
   - Discrete Kalman filter states: < 120 bytes.
   - Brain neural network & homeostatic drives: < 256 bytes.
   - Episodic memory bank (32 slots x 8 float vector): ~1.5 KB.
4. Strictly enforce zero dynamic allocations (`malloc`, `free`, `new`, `delete`) within the 60 FPS Core 1 display and kinematics execution loops.

## Alternatives Considered
- **Emulating PSRAM in flash:** Too slow, high bus latency, rapid flash wear.
- **Dynamic buffer allocation on request:** Causes heap fragmentation and unpredictable latency in real-time control loops.

## Consequences
- Total dynamic RAM usage drops to ~78 KB (leaving > 240 KB of free internal SRAM).
- Zero heap fragmentation over indefinite runtimes.
- Robust execution on standard 4MB flash ESP32-S3 boards without requiring PSRAM modules.
