/**
 * @file autonomic_engine.cpp
 * @brief Autonomous biological brainstem dynamics, Matsuoka Central Pattern Generator (CPG),
 *        metabolic homeostasis, and continuous ocular soma synthesis implementation for LoRe.
 */

#include "src/ai/autonomic_engine.h"
#include "src/math/kinematics.h"
#include "src/math/affective_engine.h"
#include "src/config/lore_config.h"
#include <math.h>
#include <string.h>

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include <Arduino.h>
#include <esp_random.h>
static inline float autonomic_rand_norm(void) {
    return ((float)(esp_random() % 2000) - 1000.0f) * 0.001f;
}
#else
#include <stdlib.h>
static inline float autonomic_rand_norm(void) {
    return ((float)(rand() % 2000) - 1000.0f) * 0.001f;
}
#endif

/* Central Pattern Generator: Respiratory/Tonic Oscillator States (Matsuoka 1985) */
static float s_u1 = 0.5f;
static float s_v1 = 0.2f;
static float s_u2 = 0.1f;
static float s_v2 = 0.1f;

/* Secondary Volitional Saccade/Blink Oscillator */
static float s_u3 = 0.1f;
static float s_v3 = 0.1f;
static float s_u4 = 0.1f;
static float s_v4 = 0.1f;

/* Homeostatic Metabolism and Motor Strain Accumulators */
static float s_metabolic_energy = 0.85f;    /* [0.0, 1.0]: Higher = energetic, Lower = drowsy/dormant */
static float s_motor_tension = 0.10f;       /* [0.0, 1.0]: Strain accumulated during stationary gaze */
static float s_spontaneous_drive = 0.0f;    /* Trigger for self-willed saccade or blink */
static bool s_spontaneous_blink_flag = false;

/* Physiological Micro-Nystagmus (Langevin Process) */
static float s_nystagmus_x = 0.0f;
static float s_nystagmus_y = 0.0f;

/* Current Actuated Soma State */
static OcularSomaState s_current_soma;

/* Parametric Morph Target Table for LoRe's 12 Biological & Expressive States */
struct ExpressionMorphTarget {
    float w_left;
    float w_right;
    float h_left;
    float h_right;
    float n_left;
    float n_right;
    float tilt_left;
    float tilt_right;
    float brow_left;
    float brow_right;
    float cheek_left;
    float cheek_right;
    float upper_lid_left;
    float upper_lid_right;
    float lower_lid_left;
    float lower_lid_right;
};

/* Morph Target Definition Table:
 * Fields: { w_l, w_r, h_l, h_r, n_l, n_r, tilt_l, tilt_r, brow_l, brow_r, cheek_l, cheek_r, ulid_l, ulid_r, llid_l, llid_r }
 *
 * Anatomical conventions:
 * - Positive brow_left (+rad) and negative brow_right (-rad) slant DOWNWARDS toward facial midline (angry/determined brow).
 * - Negative brow_left (-rad) and positive brow_right (+rad) droop DOWNWARDS toward outer lateral corners (sad/sorrow brow).
 * - lower_lid: cheek squint pushing upwards (e.g. happy crescent, smiling squint).
 * - upper_lid: palpebral droop falling downwards (e.g. sleepy, cool, suspicious).
 */
static const ExpressionMorphTarget kExpressionMorphTable[NUM_EXPRESSIONS] = {
    /* 0: EXPR_IDLE - Balanced canonical resting squircle */
    { 32.0f, 32.0f, 28.0f, 28.0f, 2.8f, 2.8f,  0.00f,  0.00f,  0.00f,  0.00f, 0.0f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f },
    /* 1: EXPR_HAPPY - Cheerful crescent with raised lower palpebral cheek planes */
    { 32.0f, 32.0f, 26.0f, 26.0f, 2.8f, 2.8f,  0.00f,  0.00f,  0.00f,  0.00f, 0.0f, 0.0f, 0.00f, 0.00f, 0.45f, 0.45f },
    /* 2: EXPR_ANGRY - Fierce inward-slanted brow plane with tensed lower palpebra */
    { 32.0f, 32.0f, 26.0f, 26.0f, 2.6f, 2.6f,  0.00f,  0.00f,  0.38f, -0.38f, 0.0f, 0.0f, 0.28f, 0.28f, 0.10f, 0.10f },
    /* 3: EXPR_SAD - Dejected outer-drooping brow plane with lowered gaze */
    { 30.0f, 30.0f, 25.0f, 25.0f, 3.0f, 3.0f,  0.00f,  0.00f, -0.30f,  0.30f, 0.0f, 0.0f, 0.25f, 0.25f, 0.05f, 0.05f },
    /* 4: EXPR_SURPRISED - Wide-open, dilated round oculi (n -> 2.0) with zero palpebral obstruction */
    { 28.0f, 28.0f, 32.0f, 32.0f, 2.0f, 2.0f,  0.00f,  0.00f,  0.00f,  0.00f, 0.0f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f },
    /* 5: EXPR_SUSPICIOUS - Narrowed critical aperture with asymmetric skeptical brow slant */
    { 34.0f, 34.0f, 18.0f, 18.0f, 3.2f, 3.2f,  0.00f,  0.00f,  0.18f, -0.05f, 0.0f, 0.0f, 0.22f, 0.25f, 0.20f, 0.20f },
    /* 6: EXPR_CURIOUS - Asymmetric raised inquisitive brow and dilated left gaze */
    { 30.0f, 32.0f, 30.0f, 24.0f, 2.4f, 2.8f, -0.05f,  0.05f, -0.15f,  0.10f, 0.0f, 0.0f, 0.00f, 0.15f, 0.00f, 0.05f },
    /* 7: EXPR_MISCHIEF - Playful asymmetric smirk with slanted brow and squinted lower lid */
    { 32.0f, 32.0f, 24.0f, 24.0f, 2.8f, 2.8f,  0.08f, -0.08f,  0.28f, -0.15f, 0.0f, 0.0f, 0.15f, 0.15f, 0.25f, 0.25f },
    /* 8: EXPR_SLEEPY - Heavy drowsy upper lids drooping over softened oculi */
    { 32.0f, 32.0f, 18.0f, 18.0f, 3.0f, 3.0f,  0.00f,  0.00f,  0.00f,  0.00f, 0.0f, 0.0f, 0.42f, 0.42f, 0.15f, 0.15f },
    /* 9: EXPR_COOL - Sunglasses swagger with horizontal top cutoff and relaxed posture */
    { 34.0f, 34.0f, 22.0f, 22.0f, 3.5f, 3.5f,  0.00f,  0.00f,  0.00f,  0.00f, 0.0f, 0.0f, 0.35f, 0.35f, 0.00f, 0.00f },
    /* 10: EXPR_DIZZY - Disoriented vestibular tilt and tissue-conserved squishy oculus */
    { 28.0f, 28.0f, 28.0f, 28.0f, 2.05f, 2.05f,  0.00f,  0.00f,  0.00f,  0.00f, 0.0f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f },
    /* 11: EXPR_CRYING - Trembling sorrowful outer droop with weeping palpebral constriction */
    { 30.0f, 30.0f, 24.0f, 24.0f, 2.8f, 2.8f,  0.00f,  0.00f, -0.32f,  0.32f, 0.0f, 0.0f, 0.30f, 0.30f, 0.20f, 0.20f }
};

/* Dynamically filtered continuous morph targets */
static float s_morph_w_l = SOMA_CANONICAL_EYE_WIDTH_PX;
static float s_morph_w_r = SOMA_CANONICAL_EYE_WIDTH_PX;
static float s_morph_h_l = SOMA_CANONICAL_EYE_HEIGHT_PX;
static float s_morph_h_r = SOMA_CANONICAL_EYE_HEIGHT_PX;
static float s_morph_n_l = SOMA_CANONICAL_SQUIRCLE_N;
static float s_morph_n_r = SOMA_CANONICAL_SQUIRCLE_N;
static float s_morph_tilt_l = 0.0f;
static float s_morph_tilt_r = 0.0f;
static float s_morph_brow_l = 0.0f;
static float s_morph_brow_r = 0.0f;
static float s_morph_cheek_l = 0.0f;
static float s_morph_cheek_r = 0.0f;
static float s_morph_upper_lid_l = 0.0f;
static float s_morph_upper_lid_r = 0.0f;
static float s_morph_lower_lid_l = 0.0f;
static float s_morph_lower_lid_r = 0.0f;

static inline float relu_f(float val) {
    return (val > 0.0f) ? val : 0.0f;
}

static inline float clamp_f(float val, float min_val, float max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

void initAutonomicEngine(void) {
    s_u1 = 0.6f;
    s_v1 = 0.2f;
    s_u2 = 0.1f;
    s_v2 = 0.1f;

    s_u3 = 0.1f;
    s_v3 = 0.1f;
    s_u4 = 0.1f;
    s_v4 = 0.1f;

    s_metabolic_energy = 0.85f;
    s_motor_tension = 0.10f;
    s_spontaneous_drive = 0.0f;
    s_spontaneous_blink_flag = false;
    s_nystagmus_x = 0.0f;
    s_nystagmus_y = 0.0f;

    s_morph_w_l = SOMA_CANONICAL_EYE_WIDTH_PX;
    s_morph_w_r = SOMA_CANONICAL_EYE_WIDTH_PX;
    s_morph_h_l = SOMA_CANONICAL_EYE_HEIGHT_PX;
    s_morph_h_r = SOMA_CANONICAL_EYE_HEIGHT_PX;
    s_morph_n_l = SOMA_CANONICAL_SQUIRCLE_N;
    s_morph_n_r = SOMA_CANONICAL_SQUIRCLE_N;
    s_morph_tilt_l = 0.0f;
    s_morph_tilt_r = 0.0f;
    s_morph_brow_l = 0.0f;
    s_morph_brow_r = 0.0f;
    s_morph_cheek_l = 0.0f;
    s_morph_cheek_r = 0.0f;
    s_morph_upper_lid_l = 0.0f;
    s_morph_upper_lid_r = 0.0f;
    s_morph_lower_lid_l = 0.0f;
    s_morph_lower_lid_r = 0.0f;

    /* Initialize baseline soma state */
    s_current_soma.left_x = SOMA_CANONICAL_LEFT_X;
    s_current_soma.left_y = SOMA_CANONICAL_CENTER_Y;
    s_current_soma.right_x = SOMA_CANONICAL_RIGHT_X;
    s_current_soma.right_y = SOMA_CANONICAL_CENTER_Y;
    s_current_soma.left_w = SOMA_CANONICAL_EYE_WIDTH_PX;
    s_current_soma.left_h = SOMA_CANONICAL_EYE_HEIGHT_PX;
    s_current_soma.right_w = SOMA_CANONICAL_EYE_WIDTH_PX;
    s_current_soma.right_h = SOMA_CANONICAL_EYE_HEIGHT_PX;
    s_current_soma.left_n = SOMA_CANONICAL_SQUIRCLE_N;
    s_current_soma.right_n = SOMA_CANONICAL_SQUIRCLE_N;
    s_current_soma.stroke_thickness = SOMA_CANONICAL_STROKE_WIDTH; /* Solid filled */
    s_current_soma.tilt_left = 0.0f;
    s_current_soma.tilt_right = 0.0f;
    s_current_soma.brow_tilt_left = 0.0f;
    s_current_soma.brow_tilt_right = 0.0f;
    s_current_soma.cheek_tilt_left = 0.0f;
    s_current_soma.cheek_tilt_right = 0.0f;
    s_current_soma.upper_lid_left = 0.0f;
    s_current_soma.upper_lid_right = 0.0f;
    s_current_soma.lower_lid_left = 0.0f;
    s_current_soma.lower_lid_right = 0.0f;
    s_current_soma.palpebral_aperture = 1.0f;
    s_current_soma.hardware_contrast = 180;
    s_current_soma.nystagmus_x = 0.0f;
    s_current_soma.nystagmus_y = 0.0f;
}

void updateAutonomicEngine(float dt_sec) {
    if (dt_sec <= 0.001f) dt_sec = 0.0166f;
    if (dt_sec > 0.10f) dt_sec = 0.10f;

    /* 1. Matsuoka Respiratory CPG Integration */
    const float tau_r = 0.38f;
    const float tau_a = 1.15f;
    const float beta = 2.5f;
    const float w = 2.6f;

    /* Basal tonic excitation scales with internal metabolic energy */
    float c_tonic = 0.80f + 0.50f * s_metabolic_energy;

    float y1 = relu_f(s_u1);
    float y2 = relu_f(s_u2);

    float du1 = (c_tonic - s_u1 - beta * s_v1 - w * y2) / tau_r;
    float dv1 = (y1 - s_v1) / tau_a;

    float du2 = (c_tonic - s_u2 - beta * s_v2 - w * y1) / tau_r;
    float dv2 = (y2 - s_v2) / tau_a;

    s_u1 += du1 * dt_sec;
    s_v1 += dv1 * dt_sec;
    s_u2 += du2 * dt_sec;
    s_v2 += dv2 * dt_sec;

    float resp_phase = y1 - y2; /* Rhythmic respiratory drive [-1.0, 1.0] */

    /* 2. Homeostatic Motor Strain & Volitional Saccade/Blink Oscillator */
    /* Stationary posture slowly builds up motor tension */
    s_motor_tension += 0.045f * dt_sec;
    if (s_motor_tension > 1.0f) s_motor_tension = 1.0f;

    /* Secondary Matsuoka pair excited by motor tension */
    const float tau_r2 = 0.55f;
    const float tau_a2 = 1.60f;
    float c_tension = 0.20f + 1.20f * s_motor_tension;

    float y3 = relu_f(s_u3);
    float y4 = relu_f(s_u4);

    float du3 = (c_tension - s_u3 - beta * s_v3 - w * y4) / tau_r2;
    float dv3 = (y3 - s_v3) / tau_a2;

    float du4 = (c_tension - s_u4 - beta * s_v4 - w * y3) / tau_r2;
    float dv4 = (y4 - s_v4) / tau_a2;

    s_u3 += du3 * dt_sec;
    s_v3 += dv3 * dt_sec;
    s_u4 += du4 * dt_sec;
    s_v4 += dv4 * dt_sec;

    s_spontaneous_drive = y3;

    /* When exploratory oscillator fires strongly (bifurcation), trigger spontaneous blink/sigh */
    if (y3 > 0.45f && s_motor_tension > 0.40f) {
        s_spontaneous_blink_flag = true;
        s_motor_tension = 0.05f; /* Tension discharged */
    }

    /* 3. Metabolic Energy Balance (Autopoiesis) */
    /* Energy burns slowly during wakefulness, slightly faster during high respiratory drive */
    float drain_rate = 0.0004f * (1.0f + 0.3f * fabsf(resp_phase));
    s_metabolic_energy -= drain_rate * dt_sec;
    if (s_metabolic_energy < 0.20f) {
        /* Basal recovery pulse during deep rest */
        s_metabolic_energy += 0.0008f * dt_sec;
    }
    s_metabolic_energy = clamp_f(s_metabolic_energy, 0.15f, 1.0f);

    /* 4. Physiological Micro-Nystagmus:
     * Zeroed to guarantee rock-solid edge stability with zero pixel flicker on 1-bit OLED */
    s_nystagmus_x = 0.0f;
    s_nystagmus_y = 0.0f;

    /* 5. Continuous Ocular Soma State Synthesis:
     * Morph targets converge smoothly towards active expression with 85 ms critically damped response */
    static const float kMorphTauSec = 0.085f;
    float alpha = 1.0f - expf(-dt_sec / kMorphTauSec);
    alpha = clamp_f(alpha, 0.0f, 1.0f);

    int expr_idx = (int)g_currentExpr;
    if (expr_idx < 0 || expr_idx >= NUM_EXPRESSIONS) expr_idx = (int)EXPR_IDLE;
    const ExpressionMorphTarget& tgt = kExpressionMorphTable[expr_idx];

    s_morph_w_l += (tgt.w_left - s_morph_w_l) * alpha;
    s_morph_w_r += (tgt.w_right - s_morph_w_r) * alpha;
    s_morph_h_l += (tgt.h_left - s_morph_h_l) * alpha;
    s_morph_h_r += (tgt.h_right - s_morph_h_r) * alpha;
    s_morph_n_l += (tgt.n_left - s_morph_n_l) * alpha;
    s_morph_n_r += (tgt.n_right - s_morph_n_r) * alpha;
    s_morph_tilt_l += (tgt.tilt_left - s_morph_tilt_l) * alpha;
    s_morph_tilt_r += (tgt.tilt_right - s_morph_tilt_r) * alpha;
    s_morph_brow_l += (tgt.brow_left - s_morph_brow_l) * alpha;
    s_morph_brow_r += (tgt.brow_right - s_morph_brow_r) * alpha;
    s_morph_cheek_l += (tgt.cheek_left - s_morph_cheek_l) * alpha;
    s_morph_cheek_r += (tgt.cheek_right - s_morph_cheek_r) * alpha;
    s_morph_upper_lid_l += (tgt.upper_lid_left - s_morph_upper_lid_l) * alpha;
    s_morph_upper_lid_r += (tgt.upper_lid_right - s_morph_upper_lid_r) * alpha;
    s_morph_lower_lid_l += (tgt.lower_lid_left - s_morph_lower_lid_l) * alpha;
    s_morph_lower_lid_r += (tgt.lower_lid_right - s_morph_lower_lid_r) * alpha;

    /* Modulated by respiratory hippus vitality pulse */
    static const float kHippusRespGain = 0.022f;  /* +/- 2.2% dynamic respiratory breathing pulse */
    static const float kHippusTonicGain = 0.018f; /* Vitality scaling with metabolic energy */
    float pulse = 1.0f + kHippusRespGain * resp_phase + kHippusTonicGain * (s_metabolic_energy - 0.50f);
    pulse = clamp_f(pulse, 0.95f, 1.05f);

    float left_w = s_morph_w_l * pulse;
    float right_w = s_morph_w_r * pulse;
    float left_h = s_morph_h_l * pulse;
    float right_h = s_morph_h_r * pulse;
    float left_n = s_morph_n_l;
    float right_n = s_morph_n_r;

    /* Stroke thickness: Always 100% solid filled */
    float stroke = SOMA_CANONICAL_STROKE_WIDTH;

    /* Listing's Law: Biomechanical ocular torsion on diagonal eccentric gaze */
    float torsion_rad = getListingTorsionAngleRad(g_currentOffsetX, g_currentOffsetY);
    float tilt_l = s_morph_tilt_l + torsion_rad;
    float tilt_r = s_morph_tilt_r + torsion_rad;

    /* Vestibular Disturbance & Biological Tissue Conservation Law for EXPR_DIZZY */
    static float s_dizzy_intensity = 0.0f;
    static float s_dizzy_phase = 0.0f;
    float target_dizzy = (g_currentExpr == EXPR_DIZZY) ? 1.0f : 0.0f;
    s_dizzy_intensity += (target_dizzy - s_dizzy_intensity) * alpha;

    float dizzy_offset_x_l = 0.0f, dizzy_offset_y_l = 0.0f;
    float dizzy_offset_x_r = 0.0f, dizzy_offset_y_r = 0.0f;

    if (s_dizzy_intensity > 0.001f) {
        static const float kDizzyOmega = 4.2f;       /* Harmonic frequency (~0.67 Hz cycle) */
        static const float kDizzyTiltAmp = 0.28f;     /* Torsional roll amplitude (~16 deg) */
        static const float kDizzyPhaseLag = 0.12f;    /* Biological neuro-muscular asymmetry lag */
        static const float kDizzySquashAmp = 0.16f;   /* Incompressible vertical strain (+/- 16%) */
        static const float kDizzyWobbleAmp = 1.8f;    /* Orbital vertigo wobble radius (px) */

        s_dizzy_phase += kDizzyOmega * dt_sec;
        if (s_dizzy_phase > 62.831853f) s_dizzy_phase -= 62.831853f;

        /* 1. Bilateral Conjugate Torsional Pendulum:
         * Alternates smoothly between outward splay (\ /) and inward splay (/ \) */
        float tilt_dizzy_l = kDizzyTiltAmp * sinf(s_dizzy_phase);
        float tilt_dizzy_r = -kDizzyTiltAmp * sinf(s_dizzy_phase + kDizzyPhaseLag);

        /* 2. Biological Tissue Conservation (Incompressibility Law: Sx = 1.0 / sqrt(Sy)):
         * As one eye squashes vertically, it bulges horizontally, conserving ocular volume */
        float s_y_l = 1.0f + kDizzySquashAmp * cosf(s_dizzy_phase);
        float s_y_r = 1.0f - kDizzySquashAmp * cosf(s_dizzy_phase + kDizzyPhaseLag);
        float s_x_l = 1.0f / sqrtf(s_y_l);
        float s_x_r = 1.0f / sqrtf(s_y_r);

        /* 3. Viscoelastic Curvature Plasticity: softens squircle towards organic oval */
        float n_mod = 0.15f * cosf(2.0f * s_dizzy_phase);

        /* 4. Asynchronous Orbital Vertigo Drift */
        dizzy_offset_x_l = cosf(s_dizzy_phase) * kDizzyWobbleAmp * s_dizzy_intensity;
        dizzy_offset_y_l = sinf(s_dizzy_phase) * kDizzyWobbleAmp * s_dizzy_intensity;
        dizzy_offset_x_r = cosf(s_dizzy_phase + 3.14159265f) * kDizzyWobbleAmp * s_dizzy_intensity;
        dizzy_offset_y_r = sinf(s_dizzy_phase + 3.14159265f) * kDizzyWobbleAmp * s_dizzy_intensity;

        /* Modulate somatic dimensions smoothly by dizzy intensity */
        left_w *= (1.0f + (s_x_l - 1.0f) * s_dizzy_intensity);
        right_w *= (1.0f + (s_x_r - 1.0f) * s_dizzy_intensity);
        left_h *= (1.0f + (s_y_l - 1.0f) * s_dizzy_intensity);
        right_h *= (1.0f + (s_y_r - 1.0f) * s_dizzy_intensity);
        left_n -= n_mod * s_dizzy_intensity;
        right_n -= n_mod * s_dizzy_intensity;
        tilt_l += tilt_dizzy_l * s_dizzy_intensity;
        tilt_r += tilt_dizzy_r * s_dizzy_intensity;
    }

    /* Hardware OLED Contrast Brightness: dynamically linked to metabolic vitality */
    int raw_contrast = 40 + (int)(175.0f * s_metabolic_energy + 25.0f * y1);
    uint8_t hw_contrast = (uint8_t)clamp_f((float)raw_contrast, 30.0f, 255.0f);

    /* Store updated soma state */
    s_current_soma.left_x = SOMA_CANONICAL_LEFT_X + dizzy_offset_x_l;
    s_current_soma.left_y = SOMA_CANONICAL_CENTER_Y + dizzy_offset_y_l;
    s_current_soma.right_x = SOMA_CANONICAL_RIGHT_X + dizzy_offset_x_r;
    s_current_soma.right_y = SOMA_CANONICAL_CENTER_Y + dizzy_offset_y_r;
    s_current_soma.left_w = left_w;
    s_current_soma.right_w = right_w;
    s_current_soma.left_h = left_h;
    s_current_soma.right_h = right_h;
    s_current_soma.left_n = left_n;
    s_current_soma.right_n = right_n;
    s_current_soma.stroke_thickness = stroke;
    s_current_soma.tilt_left = tilt_l;
    s_current_soma.tilt_right = tilt_r;
    s_current_soma.brow_tilt_left = s_morph_brow_l;
    s_current_soma.brow_tilt_right = s_morph_brow_r;
    s_current_soma.cheek_tilt_left = s_morph_cheek_l;
    s_current_soma.cheek_tilt_right = s_morph_cheek_r;
    s_current_soma.upper_lid_left = s_morph_upper_lid_l;
    s_current_soma.upper_lid_right = s_morph_upper_lid_r;
    s_current_soma.lower_lid_left = s_morph_lower_lid_l;
    s_current_soma.lower_lid_right = s_morph_lower_lid_r;
    s_current_soma.hardware_contrast = hw_contrast;
    s_current_soma.nystagmus_x = s_nystagmus_x;
    s_current_soma.nystagmus_y = s_nystagmus_y;
}

OcularSomaState getOcularSomaState(void) {
    return s_current_soma;
}

AutonomicTelemetry getAutonomicTelemetry(void) {
    AutonomicTelemetry t;
    t.respiration_phase = relu_f(s_u1) - relu_f(s_u2);
    t.metabolic_energy = s_metabolic_energy;
    t.motor_tension = s_motor_tension;
    t.spontaneous_drive = s_spontaneous_drive;
    t.nystagmus_x = s_nystagmus_x;
    t.nystagmus_y = s_nystagmus_y;
    return t;
}

void stimulateAutonomicEngine(float stimulus_energy) {
    stimulus_energy = clamp_f(stimulus_energy, 0.0f, 1.0f);
    /* Perturb membrane potentials directly */
    s_u1 += 0.40f * stimulus_energy;
    s_u3 += 0.50f * stimulus_energy;
    s_motor_tension += 0.20f * stimulus_energy;
    if (s_motor_tension > 1.0f) s_motor_tension = 1.0f;
}

bool consumeSpontaneousBlinkTrigger(void) {
    bool flag = s_spontaneous_blink_flag;
    s_spontaneous_blink_flag = false;
    return flag;
}
