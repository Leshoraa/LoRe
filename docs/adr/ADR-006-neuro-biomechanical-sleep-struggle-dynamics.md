# ADR-006: Neuro-Biomechanical Palpebral Dynamics and Volitional Sleep Struggle Physical Law

## Status
Accepted

## Context
In ADR-005, the resting aperture clamp was relaxed to $h_{\text{idle}} = 1.0 - 0.08 \cdot P_{\text{sleep}}$, which prevented artificial squinting but caused LoRe's eyes to remain nearly 92% wide open during extreme tiredness. Furthermore, sleepy human ocular demeanor is characterized by progressive palpebral drooping ("merem setengah" or varying degrees depending on brain state) followed by a struggle to remain awake ("buka-nutup menahan kantuk" / drooping and jerking awake). Rather than relying on hardcoded animation timers or static aperture states, this behavior must emerge naturally as a continuous physical law coupled to LoRe's internal agency, homeostatic drives, and conscious volition.

## Decision
1. **Volitional Vigilance Drive ($V_{\text{will}} \in [0, 1]$):**
   - Quantify LoRe's internal will to maintain wakefulness directly from the on-device TinyML micro-brain: social bonding, curiosity, mischief, playfulness, and autonomic arousal, opposed by neuro-metabolic fatigue and boredom.
   - Ground LoRe's agency in companion presence: when a human companion is near or interacting, LoRe fights significantly harder to remain awake.
2. **Equilibrium Droop Aperture ($h_{\text{droop}} \in [0.12, 0.92]$):**
   - Derive the relaxed palpebral aperture as a continuous physical equilibrium between downward fatigue/gravity torque and upward active motor tone.
   - At moderate sleep pressure ($P_{\text{sleep}} \approx 0.55$), the aperture naturally settles to $h \approx 0.45 - 0.55$ ("merem setengah" / half-closed) without hardcoded static clamps.
3. **Four-Phase Biomechanical Eyelid ODE:**
   - Model the drowsy sleep-struggle cycle as a continuous dynamical system:
     - **Viscous Sinking:** Sluggish eyelid relaxation ($\tau_{\text{droop}} = 0.70 + 0.65 P_{\text{sleep}}\text{ s}$) paired with a progressive downward head nod ($y_{\text{nod}} \to +1.5\text{px} \dots +3.0\text{px}$).
     - **Hovering:** Dwell at $h_{\text{droop}}$ with subtle organic respiratory micro-tremor ($\pm 1.8\%$).
     - **Vigilance Recovery Surge:** Phasic motor excitation burst snapping eyelids open ($\tau_{\text{snap}} \approx 110 - 160\text{ ms}$) and elastically rebounding the head upright ($y_{\text{nod}} \to 0.0\text{px}$).
     - **Effort Hold:** Sustained wakefulness window proportional to $V_{\text{will}}$, decaying back into Sinking as muscle stamina depletes.
4. **Deterministic 60 FPS Integration:**
   - Execute the dynamical equations in the Core 1 display task loop, smoothly scaling spontaneous blinks and startle responses without tick jitter or frame drops.

## Alternatives Considered
- **Scripted Animation Timers (Hardcoded Finite State Machine):** Triggering pre-recorded eyelid keyframes after fixed delay intervals. Rejected because it violates the biological agency axiom and makes LoRe behave like an automaton rather than an autonomous sentient companion.
- **Static Aperture Squinting:** Clamping $h$ to a fixed number (e.g., $h = 0.5$) whenever $P_{\text{sleep}} > 0.5$. Rejected because it looks unnatural, static, and fails to exhibit the dynamic struggle of fighting off sleep.

## Consequences
- **Positive:**
  - Highly lifelike, organic drowsy ocular demeanor with dynamic half-closed drooping and relatable sleep-fighting kinetics.
  - Fully emergent behavior driven by LoRe's internal state, personality, and relationship with its companion.
  - Passes all 12 unit tests on host and compiles cleanly within ESP32-S3 internal SRAM budget (17% SRAM).
- **Negative:**
  - Adds minor state variables to the display task, though well within the microsecond frame budget (consuming $< 0.1\%$ CPU).
