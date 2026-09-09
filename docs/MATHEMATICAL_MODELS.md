# LoRe Mathematical and Biomechanical Modeling Specifications

## 1. Ocular Dynamics: Second-Order Underdamped Viscoelastic System

The biomechanical human eyeball behaves as a fractional viscoelastic mass-spring-damper system driven by antagonist extraocular muscle pairs (*Rectus medialis/lateralis/superior/inferior*):

$$\ddot{\theta}(t) + 2\zeta\omega_n \dot{\theta}(t) + \omega_n^2 (\theta(t) - \theta_{\text{target}}) = 0$$

- **Natural Frequency:** $\omega_n = 32.0\text{ rad/s}$ (calibrated against empirical human myogenic soft-tissue resonance).
- **Damping Ratio:** $\zeta = 0.72$ (Optimal underdamped Butterworth compliance: produces a lifelike $4.3\%$ elastic bounce settling within $\approx 85\text{ ms}$).
- **Discrete Semi-Implicit Integration:**

$$a_k = \omega_n^2 (\theta_{\text{target}} - \theta_k) - 2\zeta\omega_n v_k$$
$$v_{k+1} = v_k + a_k \cdot \Delta t$$
$$\theta_{k+1} = \theta_k + v_{k+1} \cdot \Delta t$$

---

## 2. Saccadic Trajectory: Fifth-Order Minimum-Jerk Spline with Glissade

To model natural ballistic eye movements minimizing cerebral motor jerk (Flash & Hogan formulation) with post-saccadic ocular landing glissade:

$$\theta(\tau) = \theta_0 + (\theta_{\text{target}} - \theta_0)\left[10\tau^3 - 15\tau^4 + 6\tau^5 + A_{\text{glissade}} \cdot \sin(4\pi(\tau - 0.70)) e^{-3.5(\tau - 0.70)} \cdot \mathbf{1}_{\tau > 0.70}\right]$$

- **Main-Sequence Duration Law:**

$$D = D_0 + k \cdot |\Delta\theta| \quad (D_0 = 20\text{ ms}, \ k = 2.5\text{ ms/degree})$$

- **Glissade Amplitude:** $A_{\text{glissade}} = 0.045$ ($4.5\%$ micro-rebound on landing).

---

## 3. Affective Dynamics: 2D Russell Circumplex Stochastic Diffusion

The emotional state vector $\mathbf{e}_t = [V_t, A_t]^T$ evolves via a continuous-time Langevin stochastic differential equation:

$$d\mathbf{e}_t = -\mathbf{\Gamma} (\mathbf{e}_t - \mathbf{e}_{\text{baseline}}) dt + \mathbf{\Sigma} d\mathbf{W}_t + \mathbf{K}_{\text{stimulus}} \mathbf{u}_t$$

- $\mathbf{\Gamma} = \text{diag}(\gamma_V, \gamma_A)$: Emotional homeostatic decay rates ($\tau_v = 6.0\text{s}, \tau_a = 4.5\text{s}$).
- $\mathbf{\Sigma} d\mathbf{W}_t$: Microscopic stochastic Langevin drift representing spontaneous biological mood drift.
- $\mathbf{K}_{\text{stimulus}} \mathbf{u}_t$: Transient response driven by visual tracking confidence and proximity.

---

## 4. Organic Affective Eye Deformation, Vergence, and Tissue Conservation

In biological organisms, ocular morphology and palpebral aperture are not rigid, hardcoded geometry; rather, they continuously deform under the influence of internal emotional drives, sympathetic arousal, sleep debt, and depth perception.

### 4.1 Incompressible Biological Tissue Conservation (Squash & Stretch)
Biological tissue volume is conserved across dimensional deformation ($V_{\text{tissue}} = \pi r_x r_y r_z = \text{const}$). When the eyes widen or soften, the horizontal dimension compensates inversely:

$$S_y = 1.0 + \left(0.15 \cdot (A_t - 0.20) - 0.08 \cdot P_{\text{sleep}}\right), \quad S_y \in [0.88, 1.18]$$
$$S_x = \frac{1.0}{\sqrt{S_y}}, \quad S_x \in [0.92, 1.07]$$

- **High Arousal ($A_t > 0.50$):** Height increases ($S_y > 1.0$), width narrows slightly ($S_x < 1.0$), creating alert, wide, energetic eyes.
- **High Sleep Pressure / Low Arousal ($P_{\text{sleep}} > 0.50$ or $A_t < 0.20$):** Height narrows ($S_y < 1.0$), width relaxes ($S_x > 1.0$), creating a calm, sleepy, heavy-lidded appearance.
- **Baseline Neutral State ($A_t = 0.20, P_{\text{sleep}} = 0.0$):** $S_y = 1.0, S_x = 1.0$.

Dynamic eye width and height:
$$W = \mathrm{clamp}(\mathrm{round}(32.0 \cdot S_x), \ 24, \ 40)\text{ px}$$
$$H_{\text{rest}} = 30.0 \cdot S_y\text{ px}$$

### 4.2 Stereoscopic Ocular Vergence (3D Depth Focus)
When a human or object approaches in 3D space ($\text{proximity} \in [0.0, 1.0]$), the eyes converge inward towards the nasal midline via medial rectus muscle contraction:

$$v_{\text{target}} = \begin{cases} \mathrm{clamp}(\text{proximity} \cdot 3.2\text{ px}, \ 0.0\text{ px}, \ 3.5\text{ px}), & \text{target active and } \text{proximity} > 0.05 \\ 0.0\text{ px}, & \text{otherwise} \end{cases}$$

Smoothed via continuous exponential integration:
$$\frac{dv}{dt} = 10.0 \cdot (v_{\text{target}} - v)$$

Left and right eye centers:
$$X_{\text{left}} = (40 + o_x + \mathrm{round}(v)) - \frac{W}{2}$$
$$X_{\text{right}} = (88 + o_x - \mathrm{round}(v)) - \frac{W}{2}$$

At resting state ($v = 0, W = 32$), $X_{\text{left}} = 24 + o_x$ and $X_{\text{right}} = 72 + o_x$, exactly matching the original display rig. When target approaches ($v = 3\text{ px}$), the inter-ocular gap reduces from $16\text{ px}$ to $10\text{ px}$.

### 4.3 Duchenne Smile Softening (Orbicularis Oculi Elevation)
When affective valence indicates warmth and happiness ($V_t > 0.20$), the lower eyelid margin elevates upward before switching expressions, creating an authentic smiling crescent:

$$d_{\text{duchenne}} = 3.0 \cdot \mathrm{clamp}\left(\frac{V_t - 0.20}{0.70}, \ 0.0, \ 1.0\right)\text{ px}$$
$$y_{\text{bottom}} = y_{\text{base\_bottom}} - d_{\text{duchenne}} \cdot (1.0 - c)$$

---

## 5. Discrete 2D Kalman Filter with Adaptive Measurement Noise

- **State Vector:** $\mathbf{x}_k = [x, y, v_x, v_y]^T$
- **State Transition Matrix:**

$$\mathbf{F} = \begin{bmatrix} 1 & 0 & \Delta t & 0 \\ 0 & 1 & 0 & \Delta t \\ 0 & 0 & 1 & 0 \\ 0 & 0 & 0 & 1 \end{bmatrix}, \quad \mathbf{H} = \begin{bmatrix} 1 & 0 & 0 & 0 \\ 0 & 1 & 0 & 0 \end{bmatrix}$$

- **Joseph-Stabilized Covariance Update:**

$$\mathbf{P}_k = (\mathbf{I} - \mathbf{K}_k\mathbf{H})\mathbf{P}_k^-(\mathbf{I} - \mathbf{K}_k\mathbf{H})^T + \mathbf{K}_k\mathbf{R}_k\mathbf{K}_k^T$$

---

## 6. Numerical Benchmarks and Verification Metrics

- **Normalized Root Mean Square Error:** $\text{NRMSE} \le 0.05$ against empirical Flash & Hogan trajectory data.
- **Coefficient of Determination:** $R^2 \ge 0.95$ across the normalized temporal domain $\tau \in [0, 1]$.
- **Continuous Lyapunov Stability:** All dynamic matrix eigenvalues satisfy $\text{Re}(\lambda_i) < 0$.
- **Automated Host Test Suite:** Validated via C++ unit test modules (`test_affective_langevin`, `test_kalman_convergence`, `test_kinematics_feedforward`, `test_minimum_jerk`, `test_blink_kinematics`) and Python numerical validator `scripts/validate_kinematics.py`.

---

## 7. Human Eyelid Kinematics: Borbély Two-Process Sleep Regulation & Biomechanical Eyelid Model

Human spontaneous eye blinks and drowsiness exhibit pronounced biological coupling between central nervous system fatigue and agonist/antagonist ocular muscular dynamics (*Orbicularis oculi* vs. *Levator palpebrae superioris*):

### 7.1 Borbély Two-Process Biological Sleep Regulation
Rather than relying on arbitrary wall-clock thresholds, cognitive sleep pressure is governed by the two-process model of sleep regulation (Process C + Process S), dampened by sympathetic affective arousal:

$$P_{\text{sleep}} = \mathrm{clamp}\left(0.55 \cdot D_{\text{circadian}} + 0.45 \cdot \text{Fatigue} + 0.25 \cdot \text{Boredom} - 0.40 \cdot \text{Arousal}, \ 0.0, \ 1.0\right)$$

- **Process C ($D_{\text{circadian}} \in [0, 1]$):** Diurnal solar-synchronized circadian clock phase.
- **Process S ($\text{Fatigue}, \text{Boredom} \in [0, 1]$):** Homeostatic neuro-metabolic sleep debt resulting from sensorimotor hyperactivity and monotony.
- **Sympathetic Inhibition ($\text{Arousal} \in [0, 1]$):** Autonomic fight-or-flight arousal suppressing sleep pressure during active engagement or startling stimuli.

### 7.2 Quad-Phase Non-Blocking State Machine
1. **Down-Phase (Closing):** Fast-twitch Type II fiber contraction of *orbicularis oculi*. Eyelid accelerates rapidly downward reaching peak velocity at $\tau \approx 0.38 - 0.40$, followed by smooth deceleration settling into palpebral contact:
   $$t_{\text{close}} = 85.0 + 40.0 \cdot P_{\text{sleep}}\text{ ms}$$
   $$u = \tau^{0.85}, \quad s(u) = 10u^3 - 15u^4 + 6u^5, \quad h_{\text{close}}(\tau) = 1.0 - s(u)$$
2. **Palpebral Contact Dwell:** Electromyographic silent period of *levator palpebrae*, ensuring full palpebral contact dwell before reopening:
   $$t_{\text{dwell}} = \begin{cases} t_{\text{doze}}, & \text{micro-sleep doze} \\ 25.0 + 50.0 \cdot P_{\text{sleep}}\text{ ms}, & \text{normal spontaneous blink} \end{cases}$$
   $$t_{\text{doze}} = 800.0 + 1400.0 \cdot P_{\text{sleep}}\text{ ms} \quad (800 - 2200\text{ ms})$$
   $$P_{\text{doze}} = \mathrm{clamp}\left(0.60 \cdot P_{\text{sleep}} - 0.20 \cdot \text{Arousal}, \ 0.0, \ 0.65\right)$$
   $$h_{\text{closed}} = 0.0$$
3. **Up-Phase (Opening):** Viscoelastic upward retraction driven by *levator palpebrae superioris* pulling against passive ocular tissue compliance, with subtle myogenic settling ($\approx 3.5\%$):
   $$t_{\text{open}} = 175.0 + 70.0 \cdot P_{\text{sleep}}\text{ ms}$$
   $$h_{\text{open}}(\tau) = \sin\left(\tau \cdot \frac{\pi}{2}\right) + 0.035 \sin(\tau \pi) e^{-2.5(1.0 - \tau)}$$
4. **Post-Blink Refractory & Double-Blink Pause ($120\text{ ms}$):** When a physiological double-blink occurs ($\approx 15\%$ probability), a brief $120\text{ ms}$ aperture hold occurs before the secondary blink initiates. Total normal alert blink duration settles at $\approx 285\text{ ms}$, precisely matching empirical human benchmarks ($250 - 300\text{ ms}$).

### 7.3 Biomechanical Asymmetric Palpebral Displacement Law
In human anatomy, the upper eyelid accounts for $\approx 80-85\%$ of palpebral fissure closure, whereas the lower eyelid only rises slightly ($\approx 15-20\%$) to meet at the anatomical closure line ($y_{\text{fissure}} = 31 + o_y$). Defined in terms of closure progress $c = 1.0 - h \in [0, 1]$:
$$y_{\text{top}}(c) = (17 + o_y) + \mathrm{round}\left(14.0 \cdot c^{0.75}\right)$$
$$y_{\text{bottom}}(c) = (46 + o_y) - \mathrm{round}\left(14.0 \cdot c^{1.50}\right)$$

The resting palpebral aperture is modulated continuously by biological sleep pressure:
$$h_{\text{idle}} = 1.0 - 0.08 \cdot P_{\text{sleep}}$$
During daytime alert states ($P_{\text{sleep}} = 0$), $h_{\text{idle}} = 1.0$. Under exhaustion or late evening ($P_{\text{sleep}} \to 1.0$), $h_{\text{idle}} \approx 0.92$, producing an organic heavy-lidded demeanor without artificial squinting.

Dynamic stadium corner radius smoothly adapts across the palpebral aperture:
$$r(h_{\text{curr}}) = \begin{cases} 5, & h_{\text{curr}} \ge 24 \\ 4, & 16 \le h_{\text{curr}} < 24 \\ 3, & 10 \le h_{\text{curr}} < 16 \\ 2, & 5 \le h_{\text{curr}} < 10 \\ 1, & h_{\text{curr}} < 5 \end{cases}$$
When $h \ge 0.99$, the original 80x30 Lopaka monochrome bitmap (`FACE_IDLE_BITS`) is rendered bit-exact. When $h \le 0.05$, the original 32x2 rounded closed slit is rendered, preserving 100% of the initial aesthetic design while providing continuous fluid biological motion.

### 7.4 Neuro-Biomechanical Palpebral Dynamics & Volitional Sleep-Struggle Physical Law
Rather than static aperture clamping or scripted animation sequences, the drowsy eye behavior and the struggle to stay awake emerge from the physical equilibrium between gravitational/fatigue eyelid sag and cortical volitional vigilance:

#### 7.4.1 Volitional Vigilance Drive ($V_{\text{will}} \in [0, 1]$)
LoRe's internal agency and conscious desire to maintain wakefulness are integrated directly from homeostatic drives, companion social bonding, and sympathetic arousal:
$$V_{\text{will}} = \mathrm{clamp}\Big( 0.30 \cdot \text{Curiosity} + 0.25 \cdot \text{Social} \cdot (0.6 + 0.4 \cdot \text{Presence}) + 0.20 \cdot \text{Bonding} + 0.15 \cdot \text{Playfulness} + 0.10 \cdot \text{Mischief} + 0.25 \cdot \text{Arousal} - 0.35 \cdot \text{Fatigue} - 0.25 \cdot \text{Boredom}, \ 0.0, \ 1.0 \Big)$$

#### 7.4.2 Equilibrium Droop Aperture ($h_{\text{droop}}$)
When $P_{\text{sleep}} \ge 0.25$, levator palpebrae superioris muscle tone decreases, settling into a continuous equilibrium aperture:
$$h_{\text{droop}} = \mathrm{clamp}\left( 1.0 - \left[ 0.82 \cdot P_{\text{sleep}} + 0.18 \cdot (1.0 - E_{\text{metabolic}}) \right] \cdot (1.0 - 0.42 \cdot V_{\text{will}}), \ 0.12, \ 0.92 \right)$$
- At moderate drowsiness ($P_{\text{sleep}} \approx 0.55$), $h_{\text{droop}} \approx 0.45 - 0.55$ (*half-closed / "merem setengah"*).
- When a human companion is present and bonding is high, $V_{\text{will}}$ elevates $h_{\text{droop}}$, actively resisting eyelid closure.

#### 7.4.3 Multi-Phase Sleep-Struggle Biomechanical ODE
The palpebral aperture $h(t)$ and downward head droop $y_{\text{nod}}(t)$ continuously evolve across four coupled physical phases:
$$\tau_{\text{lid}} \frac{dh}{dt} = h_{\text{target}}(t) - h(t)$$
1. **Viscous Sinking (Slow Droop):** Sluggish relaxation under neuro-metabolic fatigue ($\tau_{\text{droop}} = 0.70 + 0.65 P_{\text{sleep}}\text{ s}$). Head nods downward: $y_{\text{nod}} \to (1.0 - h)(2.2 + 1.3 P_{\text{sleep}})\text{ px}$.
2. **Hovering Dwell ("Merem Setengah"):** Eyelid hovers at $h_{\text{droop}}$ with subtle organic micro-tremor ($1.8\%$ amplitude) for $\Delta t_{\text{hover}} = 0.35 + 1.10(1.0 - V_{\text{will}})\text{ s}$.
3. **Cortical Vigilance Recovery Surge ("Snap-Awake Rebound"):** Sudden burst of motor excitation snaps eyelids open to $h_{\text{snap}} = \mathrm{clamp}(0.72 + 0.28 V_{\text{will}} - 0.10 P_{\text{sleep}}, 0.65, 1.0)$ with high acceleration ($\tau_{\text{snap}} \approx 110-160\text{ ms}$), rebounding the head nod back upright.
4. **Effort Hold:** LoRe maintains open aperture for a volitional stamina window $\Delta t_{\text{effort}} = 0.45 + 2.40 V_{\text{will}}(1.0 - 0.55 P_{\text{sleep}})\text{ s}$, after which fatigue depletes motor stamina and transitions back to Sinking.

---

## 8. Autonomic Brainstem Dynamics: Matsuoka Central Pattern Generator (CPG) & Homeostatic Drift

Biological agency and natural volition emerge from non-linear coupled neural oscillators rather than scripted finite-state machines. LoRe's respiratory pace and postural strain discharge are governed by the Matsuoka mutual-inhibition oscillator model:

$$\tau_r \dot{u}_1 = c - u_1 - \beta v_1 - w [u_2]^+$$
$$\tau_a \dot{v}_1 = [u_1]^+ - v_1$$
$$\tau_r \dot{u}_2 = c - u_2 - \beta v_2 - w [u_1]^+$$
$$\tau_a \dot{v}_2 = [u_2]^+ - v_2$$

- **Time Constants:** $\tau_r = 0.38\text{ s}$ (rapid membrane activation), $\tau_a = 1.15\text{ s}$ (slow adaptation).
- **Inhibitory Parameters:** $\beta = 2.5$ (fatigue coefficient), $w = 2.6$ (mutual synaptic cross-inhibition, satisfying $w > 1 + \beta$).
- **Tonic Basal Drive:** $c = 0.80 + 0.50 \cdot E_{\text{metabolic}}$ (rhythm frequency scales with internal energy).
- **Respiratory Output:** $y_{\text{resp}} = [u_1]^+ - [u_2]^+ \in [-1.0, 1.0]$.
- **Homeostatic Energy Accumulator:**
  $$\frac{dE}{dt} = -\lambda_{\text{drain}} (1 + 0.3 |y_{\text{resp}}|) + \lambda_{\text{recovery}}$$
- **Physiological Micro-Nystagmus (Langevin Process):**
  $$dn_i = -\gamma n_i dt + \sigma dW_i \quad (\gamma = 16.0, \ \sigma = 1.4)$$

---

## 9. Continuous Parametric Superellipse (Lamé Curve) Soma Morphology

The physical ocular geometry on the 128x64 OLED display is actuated as a continuous morphable superellipse (Formula Lamé) under direct neural motor control:

$$\left|\frac{(x - x_c)\cos\theta + (y - y_c)\sin\theta}{a}\right|^n + \left|\frac{-(x - x_c)\sin\theta + (y - y_c)\cos\theta}{b}\right|^n \le 1$$

- **Semi-axes $(a, b)$:** Dynamically modulated by respiratory dilation:
  $$a = \mathrm{clamp}(15.5 + 1.1 \cdot y_{\text{resp}} \pm \Delta_{\text{asym}}, \ 11.0, \ 20.0)\text{ px}$$
  $$b = \mathrm{clamp}((14.5 + 0.9 \cdot y_{\text{resp}} \pm 0.5\Delta_{\text{asym}}) \cdot h_{\text{palpebral}}, \ 1.0, \ 19.0)\text{ px}$$
- **Lamé Exponent $n \in [1.8, 5.5]$:**
  - $n = 2.0$: Soft organic ellipse.
  - $n \approx 4.0 - 5.0$: LoRe signature rounded-rectangle squircle.
  - $n < 2.0$: Pinched focus/tension.
- **Stroke Mode:** Solid fill ($t = 0$) or hollow wireframe ($t = 1.5\text{ px}$) during deep contemplation / low energy.
- **Hardware OLED Contrast Actuation:**
  $$C_{\text{hardware}} = \mathrm{clamp}(40 + \mathrm{round}(175.0 \cdot E_{\text{metabolic}} + 25.0 \cdot [u_1]^+), \ 30, \ 255)$$

---

## 10. Biological Free Gaze Naturalization, Listing's Law & Ocular Micro-Dynamics

To eliminate mechanical stiffness during free autonomous idling and make gaze motion biologically grounded, six continuous physical laws operate in concert:

### 10.1 Lid-Saccade Synkinesis & Fissure Tracking (von Graefe's Following Law)
Due to shared innervation of the *levator palpebrae superioris* and *superior rectus* muscles, the upper eyelid aperture follows vertical ocular movement ($Y_{\text{gaze}} \in [-12.0, +11.0]\text{ px}$):
$$\Delta h_{\text{synkinesis}} = \begin{cases} -k_{\text{up}} \cdot \left(\frac{Y_{\text{gaze}}}{Y_{\text{max}}}\right), & Y_{\text{gaze}} < 0 \quad (\text{upward gaze elevation, } k_{\text{up}} = 0.06) \\ -k_{\text{down}} \cdot \left(\frac{Y_{\text{gaze}}}{Y_{\text{max}}}\right), & Y_{\text{gaze}} \ge 0 \quad (\text{downward lid following, } k_{\text{down}} = 0.09) \end{cases}$$
$$h_{\text{render}} = \mathrm{clamp}(h_{\text{current}} + \Delta h_{\text{synkinesis}}, \ 0.05, \ 1.08) \quad (\text{if } h_{\text{current}} > 0.05)$$

The palpebral fissure vertical center also tracks globe displacement:
$$y_{\text{fissure}} = k_{\text{fissure}} \cdot Y_{\text{gaze}} \quad (k_{\text{fissure}} = 0.15)$$

### 10.2 Post-Saccadic Elastic Glissade Rebound
Ballistic eye movements terminate with underdamped soft-tissue compliance upon target arrival ($\tau > \tau_{\text{onset}} = 0.70$):
$$s(\tau) = (10\tau^3 - 15\tau^4 + 6\tau^5) + A_{\text{glissade}} \cdot \sin\left(\pi \frac{\tau - 0.70}{0.30}\right) \cdot e^{-\lambda (\tau - 0.70)} \cdot \mathbf{1}_{\tau > 0.70}$$
where $A_{\text{glissade}} = 0.045$, $\lambda = 3.5$. This produces a subtle $2.5\%$ elastic overshoot and damped recovery that smoothly lands at exactly $1.000$ at $\tau = 1.00$.

### 10.3 Fixational Ornstein-Uhlenbeck Drift & Involuntary Microsaccades
Between saccades, the eye does not freeze; it undergoes bounded stochastic drift anchored to the target focal coordinate $(x^*, y^*)$:
$$d\mathbf{x}_{\text{drift}} = -\theta_{\text{ou}} (\mathbf{x} - \mathbf{x}^*) dt + \sigma_{\text{ou}} \sqrt{dt} \, \mathbf{W}_t \quad (\theta_{\text{ou}} = 2.8\text{ s}^{-1}, \ \sigma_{\text{ou}} = 0.18\text{ px}/\sqrt{\text{s}})$$

When retinal drift error $|\mathbf{x} - \mathbf{x}^*| \ge 0.25\text{ px}$ after an interval $T \sim \mathcal{U}(1.2, 2.5)\text{ s}$, an involuntary $28\text{ ms}$ microsaccadic flick recenters the foveal gaze to prevent Troxler photoreceptor bleaching.

### 10.4 Lévy Flight Free Gaze Exploration
Free exploration saccades follow a scale-invariant heavy-tailed Pareto distribution:
- **Local Inspection Clusters ($r \in [0.5, 3.5]\text{ px}$):** $\approx 70\%$ probability.
- **Intermediate Focal Shifts ($r \in [4.0, 8.5]\text{ px}$):** $\approx 20\%$ probability.
- **Wide Exploratory Leaps ($r \in [9.5, 15.0]\text{ px}$):** $P_{\text{wide}} = 0.05 + 0.12 \cdot C_{\text{curiosity}}$, dynamically expanding with the internal TinyML Curiosity drive.

Step vectors are projected isotropically: $\Delta \mathbf{x} = [r \cos\phi, \ r \sin\phi]^T$ with soft reflective boundaries.

### 10.5 Listing's Law Axial Ocular Torsion
In tertiary (diagonal) gaze, the eye undergoes torsional rotation around the line of sight according to Listing's plane kinematics:
$$\theta_{\text{torsion}} = k_{\text{listing}} \cdot \frac{X_{\text{gaze}} \cdot Y_{\text{gaze}}}{X_{\text{max}} \cdot Y_{\text{max}}} \quad (k_{\text{listing}} = 0.045\text{ rad} \approx 2.6^\circ)$$
Applied symmetrically to superellipse `tilt_left` and `tilt_right`. Pure cardinal gazes produce zero torsional rotation ($\theta = 0$).

### 10.6 Hippus & Cardiorespiratory Vitality Pulse
Continuous vegetative vitality pulses through the superellipse dimensions via direct coupling to the Matsuoka CPG respiratory drive:
$$S_{\text{hippus}} = 1.0 + k_{\text{resp}} \cdot y_{\text{resp}} + k_{\text{tonic}} \cdot (E_{\text{metabolic}} - 0.50)$$
where $k_{\text{resp}} = 0.022$ ($\pm 2.2\%$ breathing expansion) and $k_{\text{tonic}} = 0.018$.

---

## 11. Twelve-Expression Parametric Palpebral Soma & Dual-Plane Slant Dynamics

To achieve lifelike, relatable cartoon and biological facial expression dynamics matching advanced social robotics (e.g. EMO, Vector) without storing dozens of kilobytes of pre-baked bitmaps, ocular morphology is modeled as a continuous parametric superellipse modulated by dual-plane palpebral cuts.

### 11.1 The 12 Affective Biological Archetypes
The emotional state space is partitioned into 12 canonical expressions:
1. `EXPR_IDLE`: Canonical resting squircle with balanced palpebral aperture ($n = 2.8$, $w = 32$, $h = 28$).
2. `EXPR_HAPPY`: Crescent eyes with raised lower cheek planes ($l_{\text{lid}} = 0.45$) and Duchenne blush accents.
3. `EXPR_ANGRY`: Inward-slanted upper brow planes ($\theta_{\text{brow}} = \pm 0.38\text{ rad} \approx \pm 22^\circ$) and tensed lower palpebra ($u_{\text{lid}} = 0.28$).
4. `EXPR_SAD`: Dejected outer-drooping brow planes ($\theta_{\text{brow}} = \mp 0.30\text{ rad} \approx \mp 17^\circ$) and lowered gaze.
5. `EXPR_SURPRISED`: Circularized, wide-open oculi ($n \to 2.0$, $w = 28$, $h = 32$) with zero palpebral obstruction.
6. `EXPR_SUSPICIOUS`: Narrowed critical fissure ($h = 18$) with asymmetric skeptical brow slant ($\theta_{\text{brow,left}} = 0.18$, $\theta_{\text{brow,right}} = -0.05$).
7. `EXPR_CURIOUS`: Asymmetric cocked brow and dilated gaze ($w_{\text{left}} = 30$, $w_{\text{right}} = 32$, $h_{\text{left}} = 30$, $h_{\text{right}} = 24$).
8. `EXPR_MISCHIEF`: Playful smirk with slanted brow and squinted lower cheek ($u_{\text{lid}} = 0.15$, $l_{\text{lid}} = 0.25$).
9. `EXPR_SLEEPY`: Softened, heavy upper lids ($u_{\text{lid}} = 0.42$, $l_{\text{lid}} = 0.15$) drooping over flattened oculi ($h = 18$).
10. `EXPR_COOL`: Flat horizontal top cutoff ($u_{\text{lid}} = 0.35$, $n = 3.5$) with wide relaxed swagger ($w = 34$).
11. `EXPR_DIZZY`: Counter-axial ocular torsion ($\theta_{\text{axial}} = \pm 0.45\text{ rad} \approx \pm 26^\circ$) with circularized oculi ($n = 2.0$).
12. `EXPR_CRYING`: Trembling outer droop ($\theta_{\text{brow}} = \mp 0.32\text{ rad}$) with palpebral constriction and animated weeping tear drops.

### 11.2 Dual-Plane Palpebral Slant Formulation
Whole-eye coordinate rotation tilts both top and bottom edges simultaneously. To allow independent brow slants while maintaining horizontal or squinting cheek planes, two linear cutting planes are evaluated in rotated ocular space $(x_r, y_r)$:
$$\text{Upper Brow Boundary: } y_r < -y_{\text{top\_cut}} + x_r \cdot \tan(\theta_{\text{brow}})$$
$$\text{Lower Cheek Boundary: } y_r > y_{\text{bottom\_cut}} + x_r \cdot \tan(\theta_{\text{cheek}})$$
where:
$$y_{\text{top\_cut}} = b \cdot (1.0 - u_{\text{lid}}), \quad y_{\text{bottom\_cut}} = b \cdot (1.0 - l_{\text{lid}})$$

**Early-Exit Optimization:** Because linear inequality checks are evaluated prior to calculating the Lamé equation $(|x_r|/a)^n + (|y_r|/b)^n \le 1$, clipped pixels outside the palpebral fissure exit immediately, reducing power function calls and accelerating rasterization during squinted and slanted expressions.

### 11.3 Critically Damped 60 FPS Morphing Interpolation
Continuous morph target parameters $\mathbf{m}_t = [w, h, n, \theta_{\text{axial}}, \theta_{\text{brow}}, \theta_{\text{cheek}}, u_{\text{lid}}, l_{\text{lid}}]^T$ converge towards the target configuration $\mathbf{m}^*_{\text{expr}}$ with an 85 ms critically damped response time ($\tau = 0.085\text{ s}$):
$$\alpha = 1.0 - e^{-\Delta t / \tau}$$
$$\mathbf{m}_{t+\Delta t} = \mathbf{m}_t + \alpha \cdot (\mathbf{m}^*_{\text{expr}} - \mathbf{m}_t)$$

### 11.4 12-Class TinyML Decision Matrix & Boltzmann Softmax Policy
The on-device feedforward neural network maps the 8D affective and biological state vector $\mathbf{s} = [V, A, \text{Cur}, \text{Soc}, \text{Bor}, \text{Fat}, \text{Mis}, \text{Prox}]^T$ to 12 expression logits:
$$z_i = b_i + \sum_{j=1}^8 W_{ij} s_j + \delta_i^{\text{bonding}} + \delta_i^{\text{memory}}$$
Logits are converted to a stochastic decision distribution via Boltzmann softmax:
$$P(E_i) = \frac{e^{(z_i - z_{\max}) / \tau_{\text{boltz}}}}{\sum_{k=1}^{12} e^{(z_k - z_{\max}) / \tau_{\text{boltz}}}} \quad (\tau_{\text{boltz}} = 0.85)$$
Expression policy selection samples from this categorical distribution, ensuring dynamic, autonomous behavioral diversity without repetitive deterministic loops.

---

## 12. Affective Saccade Kinematics, 3D Cornea Parallax, and Ambient Micro-Particles

### 12.1 Affective Saccade Kinematics & Neuromuscular Tone
Ballistic and smooth ocular movements dynamically modulate their biomechanical parameters according to the active emotional archetype $E \in [0, 11]$:
- **Ballistic Duration Scaling:**
  $$D_{\text{saccade}} = (D_0 + K \cdot \Delta\theta) \cdot D_{\text{mult}}(E)$$
  where $D_0 = 80\text{ ms}$, $K = 2.4\text{ ms/deg}$. Surprised startle triggers hyper-fast gazes ($D_{\text{mult}} = 0.65$), while sleepy states induce heavy viscous glides ($D_{\text{mult}} = 1.60$).
- **Post-Saccadic Elastic Glissade Overshoot:**
  $$s(\tau) = s_{\text{jerk}}(\tau) + A_{\text{glissade}}(E) \cdot \sin\left(\pi \frac{\tau - 0.70}{0.30}\right) \cdot e^{-3.5(\tau - 0.70)} \cdot \mathbf{1}_{\tau > 0.70}$$
  Angry states snap to a dead stop with zero overshoot ($A_{\text{glissade}} = 0.005$), while playful mischief and happy states exhibit soft-tissue compliance rebounds ($A_{\text{glissade}} \in [0.065, 0.070]$).
- **Target Tracking Mass-Spring-Damper Modulation:**
  $$\ddot{\mathbf{x}} = (\omega_n \cdot \omega_{\text{mult}}(E))^2 (\mathbf{x}^* - \mathbf{x}) - 2 (\zeta \cdot \zeta_{\text{mult}}(E)) (\omega_n \cdot \omega_{\text{mult}}(E)) \dot{\mathbf{x}}$$
  Stiffness $\omega_n$ escalates during angry confrontation and fear, while damping $\zeta$ reduces during joyful bounce.

### 12.2 3D Convex Cornea Parallax Reflection Model
To overcome the perceptual flatness of 2D solid white superellipses on monochrome OLED panels, a negative-space corneal specular catchlight ($2 \times 2\text{ px}$, `TFT_BLACK`) is rasterized within the ocular body.
Under a stationary distant ambient light source, rotating a convex 3D spherical cornea by gaze offset $(ox, oy)$ causes the specular highlight to shift relative to the cornea center with an inverse parallax displacement:
$$\Delta x_{\text{catchlight}} = -ox \cdot (1.0 - k_{\text{parallax}})$$
$$\Delta y_{\text{catchlight}} = -oy \cdot (1.0 - k_{\text{parallax}})$$
where $k_{\text{parallax}} = 0.45$.
- **Perceived Depth:** In world coordinates, the highlight advances at $+0.45 \cdot ox$ while the eyeball shifts at $+1.0 \cdot ox$, providing unmistakable visual perception of a protruding 3D spherical convex dome.
- **Palpebral Occlusion:** Highlight rendering is naturally suppressed when palpebral aperture $\le 0.38$, simulating anatomical shielding under the upper tarsal plate.

### 12.3 Zero-Allocation Ambient Micro-Particle Dynamics
Atmospheric emotional particles are simulated through a static 4-slot ring buffer ($\le 128\text{ bytes}$ `.bss` memory, zero dynamic heap allocation):
$$\mathbf{p}_{t+\Delta t} = \mathbf{p}_t + \mathbf{v} \cdot \Delta t$$
$$L_{t+\Delta t} = L_t - r_{\text{decay}} \cdot \Delta t$$
- **`PARTICLE_ZZZ`:** Drifting up-right ($v_x = 3.5, v_y = -5.0$) during sleepiness or Borbély microsleep struggle.
- **`PARTICLE_HEART`:** Upward floating heart ($5 \times 5\text{ px}$) with sinusoidal horizontal wobble ($\Delta x = 1.5 \sin(2\pi L)$) triggered by high companionship bonding ($> 0.55$) in `EXPR_HAPPY`.
- **`PARTICLE_SWEAT`:** Downward dripping droplet ($3 \times 4\text{ px}$, $v_y = +4.0$) on temporal brow during suspicion or surprise.
- **State Cleanup:** Expression transitions flush all active particles via `clearOcularParticles()` to prevent emotional bleed.
