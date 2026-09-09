# ADR-009: Affective Saccade Kinematics, 3D Parallax Cornea Catchlight, and Ambient Micro-Particle Accents

## Status
Accepted

## Context
While the introduction of the twelve-expression parametric soma (ADR-008) provided rich, continuous morphological changes in eye shapes at 60 FPS, ocular movement and lighting remained largely static across emotional archetypes:
1. **Uniform Saccade Kinematics Across Emotional States:** In biology and animation theory, gaze movement dynamics (speed, damping, stiffness, and overshoot) vary strongly with emotional arousal and valence. An angry organism executes hyper-fast, rigid ballistic saccades without glissadic wobble; a happy organism exhibits light, springy trajectories with pronounced soft-tissue glissadic overshoot; and a sleepy organism displays sluggish, heavily damped viscous glides. Previously, LoRe utilized a single uniform kinematic profile for minimum-jerk saccades regardless of active emotion.
2. **Flat 2D Perception on Monochrome OLED:** Solid white superellipses on a high-contrast black OLED display lack spatial depth cues. Without specular corneal reflections or pupil features, the eye appears as a 2D planar sticker rather than a convex 3D spherical eyeball rotating within an ocular orbit.
3. **Absence of Autonomous Ambient Particle Accents:** Atmospheric cues such as floating "Zzz" runes during sleepiness, floating hearts during high bonding companionship, and sweat droplets during suspicion or surprise amplify emotional relatability and visual personality without consuming significant graphics memory.

## Decision
We implemented a three-pillar organicism suite for ocular naturalization:

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

2. **3D Parallax Cornea Catchlight (Specular Negative-Space Highlight):**
   Rendered a crisp $2 \times 2\text{ px}$ negative-space highlight (`TFT_BLACK`) inside the solid white cornea in `src/core/facial_renderer.cpp`.
   To produce the optical illusion of a convex 3D spherical cornea protruding from the ocular socket under a stationary virtual ambient light source, the highlight coordinate shifts relative to the eyeball center by an inverse parallax factor:
   $$\Delta x_{\text{catchlight}} = -ox \cdot (1 - k_{\text{parallax}})$$
   $$\Delta y_{\text{catchlight}} = -oy \cdot (1 - k_{\text{parallax}})$$
   where $k_{\text{parallax}} = 0.45$.
   When the gaze shifts right ($ox > 0$), the cornea rotates right while the specular glint displaces toward the left border of the cornea, creating an authentic 3D spherical depth illusion.
   - **Palpebral Occlusion:** When palpebral aperture drops below $0.38$, the highlight is smoothly occluded by the eyelid.
   - **Affective Adaptation:** In `EXPR_DIZZY`, the catchlights orbit in opposite phases; in `EXPR_SLEEPY`, the highlight shrinks to $1 \times 1\text{ px}$.

3. **Zero-Allocation Ambient Micro-Particle Accent System:**
   Introduced a static 4-slot ring buffer (`OcularParticle s_particles[4]`, 128 bytes total, zero heap allocation) in `facial_renderer.cpp`:
   - `PARTICLE_ZZZ`: Procedural $4 \times 4\text{ px}$ "Z" glyph drifting upward and rightward from the temple during `EXPR_SLEEPY` or Borbély sleep struggle.
   - `PARTICLE_HEART`: $5 \times 5\text{ px}$ heart glyph with harmonic horizontal sway floating upward during `EXPR_HAPPY` when companionship bonding level exceeds $0.55$.
   - `PARTICLE_SWEAT`: $3 \times 4\text{ px}$ teardrop dripping downward near the brow during `EXPR_SUSPICIOUS` or startle `EXPR_SURPRISED`.
   - Unified with existing animated tear streams (`EXPR_CRYING`), blush cheeks (`EXPR_HAPPY`, `EXPR_COOL`), and temporal anger veins (`EXPR_ANGRY`).
   - Particles are automatically purged via `clearOcularParticles()` during expression transitions.

## Consequences
- **Positive:**
  - Distinct visceral kinesthetics: angry gaze snaps rigidly, happy gaze bounds playfully, sleepy gaze drifts sluggishly.
  - Ocular gaze immediately gains perceived physical 3D spherical depth on a flat monochrome display.
  - Visual personality and emotional intimacy greatly enhanced through procedural atmospheric particles.
  - Zero dynamic heap allocation; strictly preserves internal SRAM budget (< 20% / < 65,536 bytes).
  - 100% passing unit test suite (13/13 tests).
- **Negative:**
  - Additional $\sim 100$ bytes of static `.bss` for the particle buffer and glissade state.
