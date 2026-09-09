# ADR-007: Biomechanical Free Gaze Naturalization, Listing's Law, and Ocular Micro-Dynamics

## Status
Accepted

## Context
While LoRe possessed 5th-order minimum-jerk saccadic trajectories and affective facial expressions, its free gaze behavior during unconstrained autonomous idling exhibited several synthetic, mechanical artifacts:
1. **Vertical Eyelid Decoupling:** The eyelids did not follow the globe vertically, violating *von Graefe's following law* (lid-saccade synkinesis driven by common innervation of superior rectus and levator palpebrae superioris).
2. **Abrupt Saccadic Landing:** Ballistic saccades terminated rigidly at the target coordinate without the viscoelastic tissue rebound (*post-saccadic glissade*) characteristic of biological ocular globes suspended in fatty connective tissue.
3. **Unbounded Fixational Drift:** Between saccades, gaze fixation used a simple unanchored Brownian random walk that lacked elastic restoring force and omitted involuntary retinal reset flicks (*microsaccades* to prevent Troxler's sensory fading).
4. **Uniform Scanning Distributions:** Free exploration target selection used uniform rectangular random boxes rather than the heavy-tailed power-law step distributions (*Lévy flight*) seen in biological gaze searching and foraging.
5. **Absence of Ocular Torsion:** Diagonal eccentric gaze movements remained completely upright, ignoring *Listing's Law* of tertiary ocular rotation.
6. **Static Rest Morphology:** Ocular squircle dimensions were mathematically rigid when resting, lacking the autonomic cardiorespiratory breathing rhythm (*hippus* and pupillary unrest).

## Decision
We implemented a complete suite of six biologically grounded ocular naturalization mechanisms adhering strictly to the zero-hardcoding principle and minimal public API design:

1. **Lid-Saccade Synkinesis & Fissure Tracking:**
   - Eyelid aperture continuously widens upon upward gaze ($\Delta h = -k_{\text{up}} \cdot y_{\text{norm}}$, $k_{\text{up}} = 0.06$) conveying alert openness, and narrows upon downward gaze ($\Delta h = -k_{\text{down}} \cdot y_{\text{norm}}$, $k_{\text{down}} = 0.09$) as the upper lid follows the cornea down.
   - The vertical center of the palpebral fissure tracks the eye vertically ($y_{\text{fissure}} = 0.15 \cdot Y_{\text{gaze}}$).
   - Zero aperture during blinks is strictly preserved ($h \le 0.05 \implies h = 0.0$).

2. **Post-Saccadic Viscoelastic Glissade:**
   - Augmented the 5th-order minimum-jerk spline with an underdamped viscoelastic tissue response for progress $\tau > 0.70$:
     $$s(\tau) = (10\tau^3 - 15\tau^4 + 6\tau^5) + A_{\text{glissade}} \cdot \sin(\pi \tau_{\text{tail}}) \cdot e^{-\lambda (\tau - 0.70)}$$
   - Generates a subtle $\approx 2.5\%$ elastic overshoot and damped recovery that lands smoothly at exactly $1.000$ at $\tau = 1.00$, eliminating mechanical servo stops.

3. **Ornstein-Uhlenbeck Fixational Drift & Microsaccades:**
   - Replaced unanchored velocity walks with a mean-reverting Ornstein-Uhlenbeck stochastic differential equation pulling back to the intended gaze anchor $(x^*, y^*)$ with restoring rate $\theta_{\text{ou}} = 2.8\text{ s}^{-1}$ and diffusion $\sigma_{\text{ou}} = 0.18\text{ px}/\sqrt{\text{s}}$.
   - Integrated periodic involuntary microsaccadic reset flicks ($28\text{ ms}$ duration) every $1.2 - 2.5\text{ seconds}$ whenever retinal drift error exceeds $0.25\text{ px}$.

4. **Lévy Flight Heavy-Tailed Gaze Exploration:**
   - Free viewing saccade amplitudes follow a three-tiered Pareto/Lévy hierarchy:
     - **Local Clusters ($r \in [0.5, 3.5]\text{ px}$):** $\approx 70\%$ probability, closely inspecting local spatial details.
     - **Intermediate Shifts ($r \in [4.0, 8.5]\text{ px}$):** $\approx 20\%$ probability, transitioning across proximate focal areas.
     - **Wide Exploratory Leaps ($r \in [9.5, 15.0]\text{ px}$):** $5\% - 17\%$ probability, dynamically boosted by on-device TinyML Curiosity drive.
   - Step vectors are projected isotropically with soft reflective boundary conditions.

5. **Listing's Law Axial Ocular Torsion:**
   - Tertiary diagonal gaze angles evoke continuous torsional rotation around the line of sight according to Listing's plane kinematics:
     $$\theta_{\text{torsion}} = k_{\text{listing}} \cdot \frac{X_{\text{gaze}} \cdot Y_{\text{gaze}}}{X_{\text{max}} \cdot Y_{\text{max}}}$$
   - Applied symmetrically to `tilt_left` and `tilt_right` ($\pm 2.6^\circ$ max), while preserving zero tilt during primary and cardinal gazes.

6. **Hippus & Autonomic Vitality Pulse:**
   - Coupled ocular superellipse semi-axes ($a, b$) with the Matsuoka CPG respiratory oscillator ($y_1 - y_2$) and metabolic energy level:
     $$S_{\text{hippus}} = 1.0 + k_{\text{resp}} \cdot (y_1 - y_2) + k_{\text{tonic}} \cdot (E_{\text{metabolic}} - 0.50)$$
   - Produces an organic $\pm 2.2\%$ vitality breathing pulsation that breathes synchronously with LoRe's internal autonomic rhythm.

## Alternatives Considered
- **Uniform Gaze Jumps:** Choosing coordinates independently across screen boundaries. Rejected as it produces unnatural ping-pong scanning patterns without focal coherence.
- **Pure Brownian Drift:** Retaining simple damping without target anchoring. Rejected as it causes steady drift away from the point of regard without physiological microsaccadic reset mechanisms.
- **Pre-baked Glissade Animations:** Storing keyframe offsets. Rejected because saccades vary dynamically from $110\text{ ms}$ to $260\text{ ms}$; an analytical minimum-jerk formulation guarantees scale invariance across arbitrary saccade amplitudes.

## Consequences
- **Positive:**
  - Markedly enhanced perceptual organicism and lifelike presence; eliminates all mechanical servo stiffness.
  - Mathematically grounded in established ocular physiology (Flash & Hogan, Martinez-Conde, Listing, von Graefe).
  - Validated by 100% passing host unit tests and zero memory regression on ESP32-S3 internal SRAM.
- **Negative:**
  - Minor trigonometric computations per frame during rendering, optimized with named constants and inline helpers to remain well below 60 FPS frame budgets.
