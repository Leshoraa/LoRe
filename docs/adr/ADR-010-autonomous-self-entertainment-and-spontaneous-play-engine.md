# ADR-010: First-Principles Continuous Sleep-Gated Restlessness, Spontaneous Solitude Play, and Ethological Agency

## Status
Accepted

## Context
Desktop companions and artificial lifeforms risk appearing mechanical or artificial if they rely on hardcoded script timers, discrete state cutscenes, or arbitrary overrides. Conversely, if an agent only responds passively to external triggers, it appears lifeless during solitude.

In biological ethology, natural animals and pets (such as cats or dogs) resting alone on a desk exhibit spontaneous autonomous self-entertainment:
1. **Endogenous Motivation:** When alone and well-rested, boredom and curiosity gradually build up, prompting restless gaze shifts, looking around the room, and playful expressions.
2. **Biological Sleep-Gating (Borbély Process S & C):** Crucially, a pet *never* fights its own biological exhaustion with artificial play routines. If the animal has high sleep debt or is circadian fatigued ($P_{\text{sleep}} > 0.6$), sleep pressure strictly dominates: eyelids droop, saccades become slow, micro-sleep dozes occur, and playfulness is suppressed.
3. **Rejection of Hardcoded State Machines & Cutscenes:** Scripted sequences (e.g. artificial 800ms prey timers, forced expression overrides, or drawing fake glowing dots) violate `AI_RULES.md` and clash with the continuous differential equations of LoRe's biomechanical and neural soma.

## Decision
We implemented a **First-Principles Continuous Sleep-Gated Restlessness Model** integrated directly into LoRe's existing mathematical engines without introducing artificial cutscene scripts or overriding the Boltzmann Softmax neural action selector:

### 1. Quadratic Biological Sleep-Gating Function
To guarantee that sleep pressure strictly dominates restlessness without threshold discontinuities:
$$\text{awake\_vitality} = \max(0.0, 1.0 - P_{\text{sleep}})$$
$$\text{sleep\_gate} = \text{awake\_vitality}^2$$

- When $P_{\text{sleep}} = 0.05$ (wide awake, fresh): $\text{sleep\_gate} \approx 0.9025$ (restlessness and playful curiosity operate at full vitality).
- When $P_{\text{sleep}} = 0.85$ (exhausted / circadian sleep): $\text{sleep\_gate} \approx 0.0225$ (restlessness is practically extinguished, allowing Borbély palpebral droop and micro-sleep dozes to proceed uninterrupted).

### 2. Continuous Langevin Affective Target Modulation
Inside `src/math/affective_engine.cpp`, during solitude (`!is_detected`), homeostatic boredom ($b$), mischief ($m$), and curiosity ($c$) continuously bias the Langevin target arousal ($A_{\text{target}}$) and valence ($V_{\text{target}}$):
$$\Delta A_{\text{target}} = (0.14 \cdot b \cdot m + 0.06 \cdot c) \cdot \text{sleep\_gate}$$
$$\Delta V_{\text{target}} = (0.08 \cdot (c - 0.20) + 0.04 \cdot m) \cdot \text{sleep\_gate}$$

This flows seamlessly through the existing stochastic differential equation:
$$\frac{d V}{d t} = \frac{V_{\text{target}} - V}{\tau_V} + \sigma_V \sqrt{dt} \cdot \eta_V$$
$$\frac{d A}{d t} = \frac{A_{\text{target}} - A}{\tau_A} + \sigma_A \sqrt{dt} \cdot \eta_A$$

The resulting shift in emotional state vector $(V, A, \dots)$ smoothly feeds into the TinyML $12 \times 8$ neural matrix and Boltzmann softmax policy (`sampleBrainExpressionPolicy()`), naturally selecting playful or curious expressions (`EXPR_CURIOUS`, `EXPR_MISCHIEF`, `EXPR_WINK`) without manual overrides.

### 3. Restlessness-Modulated Lévy Flight Saccades
In `src/math/kinematics.cpp`, autonomous ocular exploration follows a heavy-tailed Lévy flight step distribution. Solitude restlessness gently expands the probability of peripheral wide-angle glances when awake and bored:
$$P_{\text{wide}} = k_{\text{wide\_base}} + k_{\text{wide\_gain}} \cdot c + 0.08 \cdot b \cdot \text{sleep\_gate}$$
$$P_{\text{med}}  = k_{\text{med\_base}}  + k_{\text{med\_gain}}  \cdot c + 0.04 \cdot b \cdot \text{sleep\_gate}$$

When bored and well-rested, LoRe naturally casts curious wide glances around the desk environment. When drowsy, `sleep_gate` decays to zero, and gaze settles into low-amplitude local fixations with natural downward resting bias.

## Consequences
- **Positive:**
  - 100% natural, continuous biological emergence: zero hardcoded timers, zero fake cutscenes, zero sprite drawing hacks.
  - Complete harmony with Borbély sleep pressure and palpebral droop: sleep strictly suppresses restlessness.
  - Zero heap allocation and zero additional `.bss` static RAM footprint.
  - Fully covered by unit test suite (`tests/unit/test_sleep_gated_restlessness.cpp`, 14/14 tests passing).
- **Negative:**
  - Requires tuning of continuous gain coefficients ($0.14, 0.08$) to ensure emotional equilibrium across varying personality profiles.
