# ADR-004: Deep Module Architecture and Structural Separation of Concerns

## Status
Accepted

## Context
As the LoRe firmware grew to accommodate complex robotic behaviors—including 60 FPS minimum-jerk kinematics, 8 facial expressions, Russell Circumplex affective Langevin dynamics, TinyML homeostatic drives, episodic memory, ambient glance screens (Clock, Weather, Phone Notifications, Navigation HUD), and dual-band networking (Wi-Fi STA/AP and BLE NUS)—several architecture issues emerged:
1. **God Files**: `display_engine.cpp` exceeded 1,500 lines, tightly coupling 1-bit SSD1306 display hardware orchestration, rigid 2D facial rig geometry, expression transition easing, and ambient HUD screen rendering.
2. **Scattered NVS Persistence**: `wifi_manager.cpp` coupled Wi-Fi state management and DNS captive portal with unrelated NVS configuration keys (OLED brightness, weather coordinates, timezone offset, and device reboot).
3. **Duplicate Logic**: JSON key-value extraction and notification classification routines were redundantly implemented across `ble_manager.cpp`, `notification_client.cpp`, and `http_server.cpp`.
4. **Visual Decorative Noise**: Comment dividers (`====`, `----`) cluttered source files, violating clean embedded engineering practices.

## Decision
1. **Partition Display Engine into Deep Modules**:
   - `src/core/display_engine`: Serves as the display orchestrator, configuring LovyanGFX I2C bus timing, managing the 60 FPS Core 1 FreeRTOS `oledTask` loop, applying dynamic auto-brightness, and micro-shifting pixels for burn-in mitigation.
   - `src/core/facial_renderer`: Encapsulates organic affective eye deformation, stereoscopic ocular vergence, Duchenne smile softening, and natural expression transition easing.
   - `src/core/ambient_screens`: Encapsulates Clock, Weather forecast, Mobile Notification popup, and Navigation HUD turn-by-turn vector screens.
2. **Centralize Persistent Configuration (`device_config`)**:
   - Create `src/config/device_config` as the single source of truth for `lore_cfg` NVS storage (credentials, BLE advertised name, OLED brightness, weather coordinates, and timezone offset).
    - Constrain `src/net/wifi_manager` strictly to Wi-Fi STA connection, SoftAP fallback, mDNS, and DNS captive portal.
3. **Extract Shared Network Utilities & Telemetry**:
   - Create `src/net/net_utils` to centralize JSON string extraction, text wrapping, and notification application classification.
   - Create `src/net/telemetry_service` to provide a single, unified telemetry JSON serializer shared by BLE NUS and HTTP `/telemetry`.
4. **Codebase Sanitation**:
   - Remove all decorative comment banners (`====`, `----`) across the entire repository.
   - Remove all dead camera stubs and unused buffer variables.
   - Standardize all developer comments and system status messages in English.

## Alternatives Considered
- **Monolithic Single-File Core**: Retaining all display logic in a single file avoided multiple compilation units, but made code maintenance, unit testing, and modular verification unwieldy.
- **Shallow Fine-Grained Object Decomposition**: Breaking every screen element into individual classes with shallow pass-through interfaces added header bloat and indirection overhead without meaningful boundary hiding.

## Consequences
- **Positive**:
  - High cohesion with clear architectural boundaries across layers (`config`, `types`, `math`, `ai`, `core`, `net`).
  - Elimination of duplicate logic across communication channels.
  - Zero dead camera variables or unreferenced stubs.
  - Dynamic memory footprint remains lean at 56,936 bytes (17% of internal SRAM), leaving >270 KB headroom for FreeRTOS task stacks.
  - All 10 host unit tests pass with zero regressions.
- **Negative**:
  - Requires updating build scripts and header includes to reference modular directory paths.
