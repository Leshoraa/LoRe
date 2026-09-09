# ADR-008: Twelve-Expression Parametric Palpebral Soma, Dual-Plane Slant Rasterization, and 12-Class TinyML Emotional Policy

## Status
Accepted

## Context
LoRe previously supported only two baseline expressions (`EXPR_IDLE` and `EXPR_HAPPY`), with `EXPR_HAPPY` primarily relying on a static monochrome bitmap. While expressive desktop robots (such as LivingAI's EMO) exhibit rich, relatable emotional lives through dynamic eye shapes (fierce angry slants, sad dejected droops, surprised dilated circles, suspicious asymmetrical squints, curious cocked eyebrows, smug mischief, sleepy heavy lids, cool shades, dizzy disoriented spirals, and sorrowful crying), implementing dozens of discrete expressions by storing bitmaps would rapidly exhaust Flash and SRAM budgets while introducing discrete, jarring frame transitions.

Furthermore, naive whole-eye rotation ($\theta$) fails to produce natural facial expressions: tilting the entire squircle rotates the top and bottom borders simultaneously. In biological faces and cartoon ocular rigs, an angry or sad brow angles the upper eyelid plane independently while the lower eyelid / cheek plane remains horizontal or pushes upwards in a smiling squint.

## Decision
We implemented a fully mathematical, continuous parametric soma architecture supporting 12 rich biological and expressive archetypes without storing additional bitmap frames or consuming extra dynamic SRAM:

1. **Expansion to 12 Expressive & Biological States:**
   Expanded the `Expression` enum from 2 to 12 distinct affective states:
   - `EXPR_IDLE` (0): Canonical resting squircle modulated by respiratory hippus.
   - `EXPR_HAPPY` (1): Upward-squinting crescent with raised lower palpebral cheek planes and blush accents.
   - `EXPR_ANGRY` (2): Inward-slanted brow plane with tensed lower palpebra and anger vein accent.
   - `EXPR_SAD` (3): Dejected outer-drooping brow plane with lowered gaze.
   - `EXPR_SURPRISED` (4): Wide-open, circularized oculi ($n \to 2.0$) with zero palpebral obstruction.
   - `EXPR_SUSPICIOUS` (5): Narrowed critical aperture with asymmetric skeptical brow slant.
   - `EXPR_CURIOUS` (6): Asymmetric raised inquisitive brow and dilated gaze.
   - `EXPR_MISCHIEF` (7): Playful asymmetric smirk with slanted brow and squinted lower lid.
   - `EXPR_SLEEPY` (8): Heavy drowsy upper lids drooping over softened oculi.
   - `EXPR_COOL` (9): Sunglasses swagger with horizontal top cutoff and relaxed posture.
   - `EXPR_DIZZY` (10): Disoriented counter-axial torsion with circularized oculi.
   - `EXPR_CRYING` (11): Trembling sorrowful outer droop with weeping palpebral constriction and animated tear accents.

2. **Dual-Plane Palpebral Slant Formulation:**
   Extended `OcularSomaState` and `renderOneEyeSuperellipse` with independent upper (brow) and lower (cheek) cutting planes:
   $$\text{Upper Brow Cut: } y_r < -y_{\text{top\_cut}} + x_r \cdot \tan(\theta_{\text{brow}})$$
   $$\text{Lower Cheek Cut: } y_r > y_{\text{bottom\_cut}} + x_r \cdot \tan(\theta_{\text{cheek}})$$
   where $y_{\text{top\_cut}} = b \cdot (1 - u_{\text{lid}})$ and $y_{\text{bottom\_cut}} = b \cdot (1 - l_{\text{lid}})$.
   Crucially, these linear tests execute inside the coordinate transform loop *prior* to evaluating the computationally demanding superellipse equation $(|x_r|/a)^n + (|y_r|/b)^n \le 1$. Pixels outside the palpebral aperture early-exit immediately, accelerating rasterization during squinted and slanted expressions.

3. **Continuous 60 FPS Morph Target Synthesis:**
   Rather than snapping instantaneously between expressions, `autonomic_engine.cpp` integrates a `ExpressionMorphTarget` table. Active parameters $(w, h, n, \theta_{\text{brow}}, \theta_{\text{cheek}}, u_{\text{lid}}, l_{\text{lid}}, \theta_{\text{axial}})$ converge exponentially towards target values with a critically damped 85 ms time constant ($\tau = 0.085\text{ s}$):
   $$\alpha = 1.0 - e^{-\Delta t / \tau}$$
   The resulting continuous geometry is subsequently modulated by Matsuoka CPG respiratory breathing pulses, stereoscopic vergence ($v$), and Listing's Law ocular torsion.

4. **12x8 TinyML Decision Matrix & Boltzmann Softmax Policy:**
   Expanded the on-device feedforward neural weights to $12 \times 8$ and biases to 12 rows, mapping the 8D affective/homeostatic state $[V, A, \text{Cur}, \text{Soc}, \text{Bor}, \text{Fat}, \text{Mis}, \text{Prox}]$ to 12 Boltzmann softmax decision logits:
   $$P(E_i) = \frac{e^{(z_i - z_{\max}) / \tau_{\text{boltz}}}}{\sum_j e^{(z_j - z_{\max}) / \tau_{\text{boltz}}}}$$

5. **12x12 Associative Episodic Memory Matrix:**
   Expanded the associative memory resonance matrix `s_associative_matrix` to $12 \times 12$. Replaying stored past experiences boosts compatible emotional logits while suppressing dissonant affective states.

6. **Full-Spectrum Web UI Expressive Controller:**
   Updated `web_ui.h` with a 4-column responsive grid containing buttons for all 12 expressions, styled with Material 3 expressive hover/active morphing shapes matching the geometry of each emotion.

## Alternatives Considered
- **Pre-baked 1-bit Bitmaps:** Storing 12+ monochrome bitmap frames for left and right eyes. Rejected because bitmaps cannot continuously interpolate at 60 FPS, cannot scale smoothly with gaze vergence/squash-and-stretch, and would waste valuable Flash storage.
- **Whole-Eye Coordinate Rotation Only:** Rotating the entire eye bounding box. Rejected because biological eyebrows and cheeks angle independently without rotating the entire eyeball.
- **Bézier Curve Clipping:** Evaluating cubic Bézier curves for eyelid boundaries. Rejected due to excessive floating-point evaluation overhead per pixel on ESP32-S3. Linear dual-plane slant lines achieve high visual fidelity with simple multiplication and addition.

## Consequences
- **Positive:**
  - Rich, expressive emotional palette matching modern interactive companions (EMO, Vector).
  - 100% continuous, organic transitions at 60 FPS without frame popping.
  - Zero addition to dynamic heap or internal SRAM (< 20% limit strictly maintained).
  - Rasterization performance is faster during squinted/slanted expressions due to early-exit clipping.
  - 100% passing host algorithmic unit tests.
- **Negative:**
  - Slightly higher code footprint in Flash due to the 12x8 weight matrix and 12x12 associative memory matrix (well within 4MB Flash budget).
