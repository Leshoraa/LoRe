# ADR-002: Decoupling Vision Pipeline to Autonomous Gaze Engine

## Status
Accepted

## Context
In the original KoRe system, target tracking was driven by an active computer vision pipeline (`camera_pipeline.cpp`) acquiring VGA frames from an OV2640 camera over a parallel DVP bus, performing 8x spatial downscaling to 80x60 in SRAM, calculating differential motion history images (MHI), and computing spatial moments.

In LoRe, no camera hardware is installed on the device. However, the system must retain its expressive, lifelike ocular dynamics, biological micro-movements, affective state transitions, and responsive behavior without the overhead, power draw, and memory consumption of a camera sensor.

## Decision
1. Eliminate all physical camera hardware dependencies (`esp_camera.h`, OmniVision SCCB register manipulation, DVP pin configurations).
2. Encapsulate target coordination into a deep module: `src/core/gaze_engine.h` and `src/core/gaze_engine.cpp`.
3. Drive ocular movements autonomously via:
   - 5th-order minimum-jerk saccade polynomials (Flash & Hogan formulation).
   - Second-order mass-spring-damper gaze kinetics ($\omega_n = 32.0\text{ rad/s}, \zeta = 0.72$).
   - Statistical distribution of organic saccades (60% micro-shifts, 28% peripheral glances, 12% wide exploration).
   - Personality traits and circadian energy modulation.
4. Provide an interactive virtual target interface via HTTP (`POST /set_gaze`) and BLE companion commands, enabling remote steering of LoRe's gaze from a web browser or mobile app.

## Alternatives Considered
- **Dummy camera driver stub:** Keeping `esp_camera` dummy stubs returning empty frames. Rejected under Rule 9 and Rule 88 (Avoid Trash Code) because it maintains dead code, wastes CPU cycles, and adds unnecessary compilation dependencies.
- **Pure static gaze:** Freezing eye positions to center when no camera is attached. Rejected because it destroys the lifelike character and affective expressions of the robot.

## Consequences
- Reduces firmware binary size by over 350 KB.
- Eliminates camera-related thermal dissipation and current draw (~120 mA reduction).
- Frees internal SRAM previously reserved for differential image buffers and downscaling maps.
