# Changelog

All notable changes to the LoRe firmware project will be documented in this file.

## [1.5.0] - 2026-09-10

### Added
- 128 Biomechanical Micro-Expressions & Nonlinear Viscoelastic Dynamics (`src/core/micro_expression_engine.h`, `micro_expression_engine.cpp`):
  - Comprehensive biological taxonomy of 128 procedural micro-expressions organized into 8 affective categories (Cognitive, Affectionate, Playful, Startle, Skepticism, Melancholy, Irritation, Drowsiness), with 16 distinct nuances each.
  - Zero Dynamic Heap Allocation (ADR-003 compliance): all 128 archetype definitions stored in Flash `.rodata` (~2.5 KB), with runtime engine consuming < 64 bytes of internal SRAM.
  - 5th-Order Minimum-Jerk Activation ($10\tau^3 - 15\tau^4 + 6\tau^5$) providing smooth, continuous acceleration with zero velocity/acceleration impulses at boundaries.
  - Apex Dwell Kinetics with subtle respiratory hippus and stochastic micro-tremor modulation.
  - Viscoelastic Relaxation ODE: underdamped second-order mass-spring-damper ($\omega_n = 18.0\text{ rad/s}, \zeta = 0.85$) simulating biological tissue hysteresis, skin elasticity, and soft glissadic decay back to canonical macro-state.
  - Volume-Conserving Biological Tissue Incompressibility ($S_x = 1/\sqrt{S_y}$ or $S_y = 1/\sqrt{S_x}$) preserving ocular organ volume under deformation.
  - Strict Anatomical Safety Clamps: limits perturbations so expressions never distort the squircle beyond natural bounds ($W \in [24, 38]$, $H \in [16, 34]$, $n \in [2.0, 4.6]$, brow tilt $\le \pm 23^\circ$, cheek squint $\le 0.30$, upper lid $\le 0.65$, lower lid $\le 0.60$), preventing uncanny valley deformations.
  - Spontaneous Autonomous Triggering: Poisson/Langevin arrival driven by affective valence, arousal, and homeostatic curiosity/boredom drives.
  - Web UI & REST API Integration: `POST /api/micro_expr` endpoint and responsive Web Dashboard control panel with category filtering, 128-item dropdown, trigger/stop buttons, and live intensity slider.
  - Telemetry Serialization: real-time JSON snapshot updated with active `micro_id`, `micro_name`, and `micro_progress`.
  - Algorithmic Host Unit Test Suite: `tests/unit/test_micro_expressions.cpp` integrated into `scripts/run_tests.sh` (15/15 tests passing).
  - Documentation: Architecture Decision Record `docs/adr/ADR-011-one-hundred-twenty-eight-biomechanical-micro-expressions.md` and Section 13 in `docs/MATHEMATICAL_MODELS.md`.

## [1.4.0] - 2026-09-10

### Added
- First-Principles Continuous Sleep-Gated Restlessness & Spontaneous Solitude Play:
  - 100% endogenous behavioral agency driven purely by homeostatic drives, Langevin emotional diffusion, and circadian rhythm without external web or notification dependency.
  - Quadratic Biological Sleep-Gating Function ($\text{sleep\_gate} = \max(0.0, 1.0 - P_{\text{sleep}})^2$): ensures Borbély sleep pressure (Process S + Process C) strictly inhibits restlessness and suppresses artificial play during fatigue or nighttime.
  - Continuous Langevin Affective Modulation: boredom, mischief, and curiosity continuously modulate target arousal and valence ($A_{\text{target}}, V_{\text{target}}$) without artificial jumps or cutscene scripts.
  - Neural Matrix Policy Integration: natural Boltzmann Softmax neural action selector (`sampleBrainExpressionPolicy()`) selects playful/curious expressions without forced overrides.
  - Restlessness-Modulated Lévy Flight Saccades: wide exploratory gaze glances naturally expand when bored and awake, collapsing to calm fixations when drowsy.
  - Zero heap allocation and zero additional static `.bss` RAM footprint.
  - Algorithmic host unit test suite `tests/unit/test_sleep_gated_restlessness.cpp` integrated into `scripts/run_tests.sh` (14/14 tests passing).
  - Architecture Decision Record `docs/adr/ADR-010-autonomous-self-entertainment-and-spontaneous-play-engine.md`.

## [1.3.0] - 2026-09-10

### Refactored
- Neuro-Biomechanical Physical Law for `EXPR_DIZZY`: replaced the ad-hoc Archimedean spiral line-drawer (`@_@`) with a first-principles continuous physical model:
  - Bilateral Conjugate Torsional Pendulum ($\Theta_{\text{dizzy}} = \pm 16^\circ$, $\omega = 4.2\text{ rad/s}$, $\phi_{\text{lag}} = 0.12\text{ rad}$) producing dynamic splay oscillation between `\ /` and `/ \`.
  - Biological Tissue Conservation Law ($S_x = 1 / \sqrt{S_y}$, $\kappa_{\text{squash}} = 0.16$) for volume-conserving out-of-phase squishy blob dynamics.
  - Viscoelastic Curvature Plasticity ($n \in [1.9, 2.2]$) relaxing the squircle into an organic squishy oval.
  - Unified 60 FPS Superellipse Pipeline: eliminated `renderDizzySpiralEye` and special-case rendering branches in `facial_renderer.cpp` in compliance with `AI_RULES.md` (Rule 2, Rule 46 & Rule 47).
  - Compact status bar mini-eye icon updated to tilted squishy capsules with subtle splay notches.

## [1.2.0] - 2026-09-09

### Added
- Organic Affective Eye Deformation & Tissue Conservation: biological incompressibility model ($S_x = 1 / \sqrt{S_y}$) driven continuously by emotional arousal ($A_t$) and sleep pressure ($P_{\text{sleep}}$), organically scaling eye width and height without rigid geometry.
- Stereoscopic Ocular Vergence (3D Depth Focus): medial rectus convergence shifting eyes inward by up to $3.5\text{ px}$ based on target proximity ($v \in [0.0, 3.5]\text{ px}$).
- Duchenne Smile Softening: continuous lower eyelid elevation ($d_{\text{duchenne}} \in [0, 3]\text{ px}$) driven by positive emotional valence ($V_t > 0.20$), softening into an organic smiling crescent before transitioning to full happy expression.
- Split-eye independent bitmap rendering for `EXPR_HAPPY` (`FACE_HAPPY_EYE_LEFT_BITS`, `FACE_HAPPY_EYE_RIGHT_BITS`) to support stereoscopic vergence.
- Borbély's Two-Process Biological Sleep Regulation Model in `brain_engine`: integrates Circadian Process C ($D_{\text{circadian}}$), Homeostatic Sleep Debt Process S (`fatigue` + `boredom`), and sympathetic affective arousal inhibition into a continuous sleep pressure metric ($P_{\text{sleep}}$).
- Dynamic micro-sleep doze duration ($t_{\text{doze}} \in [800\text{ ms}, 2200\text{ ms}]$) scaled continuously by physiological sleep debt.
- Stochastic micro-sleep decision policy (`sampleMicroSleepDecision`) owned by the cognitive AI layer.
- Unit test coverage for Borbély Two-Process dynamics, ocular vergence, and volume-conserving tissue deformation in `test_brain_inference.cpp` and `test_kinematics_feedforward.cpp`.

### Refactored
- De-hardcoded display & facial renderer layers: replaced rigid static bitmaps and discarded parameters (`(void)vergence; (void)scale;`) with continuous affective deformation while preserving 100% bit-exact reproduction when in baseline resting state ($S = 1.0, v = 0, h \ge 0.99$).
- Eliminated static `1500.0f` ms timers, hardcoded probability thresholds, and wall-clock checks from `display_engine.cpp`, delegating all behavioral sleep regulation to `brain_engine` in accordance with `AI_RULES.md` (Rule 46 & Rule 47).
- Continuous biological resting aperture scaling ($h_{\text{idle}} = 1.0 - 0.08 \cdot P_{\text{sleep}}$) and biomechanical head nod offset ($+2.0\text{ px}$ to $+3.5\text{ px}$) during dozing.

## [1.1.0] - 2026-09-08

### Added
- Non-blocking 4-phase human eye blink state machine (`BLINK_IDLE_STATE`, `BLINK_CLOSING_STATE`, `BLINK_CLOSED_STATE`, `BLINK_OPENING_STATE`).
- Biomechanically calibrated human blink easing curves (`blinkCloseEase`, `blinkOpenEase`) matching *orbicularis oculi* and *levator palpebrae* physiology.
- Asymmetric palpebral displacement law with upper-eyelid dominance ($80\%$ downward early lead) and dynamic stadium corner radius ($r \in [1, 5]$).
- Circadian drowsiness integration: natural heavy-lidded resting aperture ($h \approx 0.92$), languid blinks, and periodic 1.5s peaceful micro-sleep dozes during late evening (23:00–01:00).
- High-precision microsecond frame pacing (`delayMicroseconds` + `taskYIELD()`) on Core 1 eliminating FreeRTOS tick jitter and enforcing rock-solid 60.0 FPS.
- New host unit test module `test_blink_kinematics.cpp` integrated into `scripts/run_tests.sh` (10 tests total).

### Fixed
- Fatal lwIP assertion crash (`assert failed: udp_new_ip_type ... Required to lock TCPIP core functionality!`) by removing obsolete NetBIOS (`AsyncUDP`) and delegating strictly to thread-safe mDNS.
- Increased `ntfyTask` initial connection delay to 6.0 seconds to allow Wi-Fi association, DHCP, and initial SNTP time sync to settle before opening persistent streaming sockets.
- Removed unwanted 2x2 debug white pixel indicator (`drawActiveIndicator`) from top-right corner of OLED display canvas.
- Corrected deep sleep activation check (`s_oled_wake_temporary_until_ms`) so device smoothly enters low-power sleep (OLED off, CPU 80 MHz) between 01:00 and 05:30, waking automatically at sunrise / 05:30.

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
