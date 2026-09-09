# ADR-005: Biomechanical Eye Blinking, Microsecond Frame Pacing, and Circadian Deep Sleep

## Status
Accepted

## Context
During user testing and dynamic evaluation of LoRe on the ESP32-S3 SuperMini hardware, four interrelated architectural and behavioral issues were observed:
1. **Unnatural & Squinted Eyelids**: The eyelid animation suffered from unnatural kinematics, and during late evening hours an arbitrary clamp forced eye aperture down to $h = 0.28$, resulting in an unintended static squinting ("mata sipit") rather than natural human sleepiness.
2. **Micro-Jitter & Lack of Fluidity**: The eyelid motion and display refresh exhibited visible stutter and failed to sustain perceived 60 FPS fluidity. Investigation revealed that FreeRTOS tick rate (`configTICK_RATE_HZ = 100`, 10 ms per tick) resulted in quantization error in `vTaskDelay`, causing frame pacing jitter between 0 ms and 20 ms.
3. **lwIP Fatal Assertion Crash**: An assertion failure occurred in the lwIP UDP subsystem (`assert failed: udp_new_ip_type ... Required to lock TCPIP core functionality!`) caused by NetBIOS invoking non-thread-safe UDP calls concurrently with Wi-Fi/SNTP network initialization.
4. **Circadian Deep Sleep Wake-Lock Glitch**: A wake-lock check unconditionally refreshed `s_oled_wake_temporary_until_ms` whenever `g_recon_state == STATE_ACTIVE`, preventing the OLED panel from entering low-power sleep between 01:00 and 05:30. Furthermore, a rogue 2x2 debug white pixel indicator was active in the top-right corner of the display.

## Decision
1. **Biomechanical 4-Phase Eyelid State Machine**:
   - Model spontaneous blinking via a non-blocking 4-phase state machine: `BLINK_IDLE_STATE`, `BLINK_CLOSING_STATE`, `BLINK_CLOSED_STATE`, and `BLINK_OPENING_STATE`.
   - Calibrate closure to 85 ms with high initial acceleration ($u = \tau^{0.85}$), palpebral contact dwell to 25 ms, and viscoelastic reopening to 175 ms (total normal cycle $\approx 285\text{ ms}$, precisely matching empirical human benchmarks).
   - Implement an asymmetric palpebral displacement law where the upper eyelid dominates ($80-85\%$ displacement) and the lower eyelid rises slightly ($15-20\%$) to meet at $y_{\text{fissure}} = 31$, adapting stadium corner radius dynamically ($r \in [1, 5]$).
   - Guarantee bit-exact rendering of the original 80x30 Lopaka bitmap when $h \ge 0.99$ and the original 32x2 rounded closed slit when $h \le 0.05$.
2. **Circadian Drowsiness & Deep Sleep Lifecycle**:
   - Replace the artificial squint clamp ($h=0.28$) with continuous circadian aperture modulation: $h_{\text{idle}} = 1.0 - 0.08 \cdot D_{\text{circadian}}$ ($h \approx 0.92$ late at night).
   - Drowsiness stretches closing to 125 ms, opening to 245 ms, and triggers periodic 1.5-second peaceful micro-sleep dozes (`s_is_drowsy_doze`).
   - Enable automated deep sleep between 01:00 and 05:30 / sunrise: power off OLED panel (`lcd.sleep()`, 0xAE display off), scale CPU down to 80 MHz (`CPU_FREQ_SLEEP_MHZ`), and wake automatically at 05:30 / sunrise with smooth eyelid opening.
   - Cleanly remove the top-right 2x2 white dot debug indicator.
3. **Core 1 Microsecond Frame Pacing**:
   - Replace FreeRTOS `vTaskDelay` on the Core 1 display task with high-resolution microsecond pacing: `micros()` tracking against `FRAME_BUDGET_ACTIVE_US = 16666`, utilizing `delayMicroseconds()` for residual budgets $> 1.5\text{ ms}$ paired with `taskYIELD()`.
   - Preserves rock-solid 60.0 FPS rendering without timer tick jitter.
4. **Remove Unstable NetBIOS Dependency**:
   - Remove NetBIOS (`NBNS.begin`) and rely exclusively on mDNS (`MDNS.begin("lore")`), which conforms to ESP-IDF thread safety and eliminates the lwIP TCPIP core lock assertion crash.

## Alternatives Considered
- **Symmetric Linear Eyelid Clipping**: Simply cropping lines from the top and bottom of the bitmap equally. Rejected because it looks mechanical, clips the eyeball unnaturally, and destroys biological plausibility.
- **Hardware FreeRTOS Tick Rate Increase (1000 Hz)**: Modifying `configTICK_RATE_HZ` to 1000 in ESP-IDF. Rejected because it increases RTOS context switching overhead across both cores and is not standard in Arduino core builds.
- **Locking lwIP Core around NetBIOS**: Wrapping NetBIOS calls with `sys_lock_tcpip_core()`. Rejected because NetBIOS is redundant when mDNS is already operational, and keeping unstable legacy libraries violates Rule 41 (Replace Unstable Code).

## Consequences
- **Positive**:
  - Silky-smooth, biological eye blinks and gaze movements at a deterministic 60.0 FPS.
  - Natural late-evening drowsiness and automated night deep sleep preventing OLED burn-in.
  - Complete elimination of lwIP assertion crashes and reboot loops.
  - Zero debug artifacts on the display canvas.
  - All 10 host unit tests pass, validating mathematical and kinematic models.
- **Negative**:
  - Core 1 spends minimal idle intervals in fine microsecond delays rather than deep OS idle, but Core 1 is dedicated solely to display and ocular synthesis.
