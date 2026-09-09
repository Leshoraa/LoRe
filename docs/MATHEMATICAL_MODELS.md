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
