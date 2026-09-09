# ADR-009: Affective Saccade Kinematics and Ambient Micro-Particle Accents

## Status
Accepted

## Context
While the introduction of the twelve-expression parametric soma (ADR-008) provided rich, continuous morphological changes in eye shapes at 60 FPS, ocular movement dynamics remained static across emotional archetypes:
1. **Uniform Saccade Kinematics Across Emotional States:** In biology and animation theory, gaze movement dynamics (speed, damping, stiffness, and overshoot) vary strongly with emotional arousal and valence. An angry organism executes hyper-fast, rigid ballistic saccades without glissadic wobble; a happy organism exhibits light, springy trajectories with pronounced soft-tissue glissadic overshoot; and a sleepy organism displays sluggish, heavily damped viscous glides. Previously, LoRe utilized a single uniform kinematic profile for minimum-jerk saccades regardless of active emotion.
2. **Absence of Autonomous Ambient Particle Accents:** Atmospheric cues such as floating "Zzz" runes during sleepiness, floating hearts during high bonding companionship, and sweat droplets during suspicion or surprise amplify emotional relatability and visual personality without consuming significant graphics memory.

## Decision
We implemented two core visual and biomechanical enhancements for ocular naturalization:

1. **Affective Saccade Kinematics Modulation Table:**
   Extended `src/math/kinematics.h` and `src/math/kinematics.cpp` with an affective kinematic profile table `kAffectiveKinematicTable[NUM_EXPRESSIONS]` mapping each of the 12 emotional archetypes to four dynamic parameters:
   - $\omega_{\text{mult}}$: Natural frequency multiplier for extraocular muscle stiffness and velocity.
   - $\zeta_{\text{mult}}$: Damping ratio multiplier modulating springiness vs. rigid stopping.
   - $D_{\text{mult}}$: Ballistic saccade duration multiplier ($0.65\times$ in surprised startle, $1.60\times$ in sleepy drag).
   - $A_{\text{glissade}}$: Post-saccadic glissade rebound gain ($0.005$ in rigid angry settling, $0.070$ in playful mischief).

   Active parameters smoothly interpolate at 60 FPS towards the target expression profile with a time constant of $\sim 83\text{ ms}$ ($\text{rate} \approx 12.0\text{ rad/s}$):
   $$\alpha_k = 1.0 - e^{-12.0 \cdot \Delta t}$$
   $$P_{\text{active}} \leftarrow P_{\text{active}} + (P_{\text{target}} - P_{\text{active}}) \cdot \alpha_k$$

   In target-tracking gaze dynamics, natural frequency and damping ratio are directly scaled:
   $$\omega_n = \omega_{\text{personality}} \cdot \omega_{\text{mult}}$$
   $$\zeta = \zeta_{\text{personality}} \cdot \zeta_{\text{mult}}$$

2. **Zero-Allocation Ambient Micro-Particle Accent System:**
   Introduced a static 4-slot ring buffer (`OcularParticle s_particles[4]`, 128 bytes total, zero heap allocation) in `facial_renderer.cpp`:
   - `PARTICLE_ZZZ`: Procedural $4 \times 4\text{ px}$ "Z" glyph drifting upward and rightward from the temple during `EXPR_SLEEPY` or Borbély sleep struggle.
   - `PARTICLE_HEART`: $5 \times 5\text{ px}$ heart glyph with harmonic horizontal sway floating upward during `EXPR_HAPPY` when companionship bonding level exceeds $0.55$.
   - `PARTICLE_SWEAT`: $3 \times 4\text{ px}$ teardrop dripping downward near the brow during `EXPR_SUSPICIOUS` or startle `EXPR_SURPRISED`.
   - Unified with existing animated tear streams (`EXPR_CRYING`), blush cheeks (`EXPR_HAPPY`, `EXPR_COOL`), and temporal anger veins (`EXPR_ANGRY`).
   - Particles are automatically purged via `clearOcularParticles()` during expression transitions.

## Alternatives Considered
- **Negative-Space Corneal Specular Catchlights (Black Dots Inside Eyes):** Evaluated rendering a $2 \times 2\text{ px}$ black negative-space glint with inverse parallax. Discarded based on visual evaluation: on 1-bit monochrome OLED displays, black pixels inside a solid white eye body resemble visual artifacts or dead pixels rather than specular glints, compromising the clean, friendly aesthetic of the robot's eyes. Pure solid white superellipses provide substantially cleaner cartoon appeal.

## Consequences
- **Positive:**
  - Distinct visceral kinesthetics: angry gaze snaps rigidly, happy gaze bounds playfully, sleepy gaze drifts sluggishly.
  - Visual personality and emotional intimacy greatly enhanced through procedural atmospheric particles.
  - Preserves clean solid-white ocular aesthetic without distracting black pupil dots.
  - Zero dynamic heap allocation; strictly preserves internal SRAM budget (< 20% / < 65,536 bytes).
  - 100% passing unit test suite (13/13 tests).
- **Negative:**
  - Additional $\sim 100$ bytes of static `.bss` for the particle buffer and glissade state.
