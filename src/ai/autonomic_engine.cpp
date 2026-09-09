/**
 * @file autonomic_engine.cpp
 * @brief Autonomous biological brainstem dynamics, Matsuoka Central Pattern Generator (CPG),
 *        metabolic homeostasis, and continuous ocular soma synthesis implementation for LoRe.
 */

#include "src/ai/autonomic_engine.h"
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

/* Injected Affective and Homeostatic Context */
static AutonomicAffectiveContext s_affective_ctx = {
    .valence = 0.0f,
    .arousal = 0.0f,
    .fatigue = 0.0f,
    .curiosity = 0.0f,
    .mischief = 0.0f,
    .boredom = 0.0f,
    .vergence_px = 0.0f
};

void setAutonomicAffectiveContext(const AutonomicAffectiveContext* ctx) {
    if (ctx) {
        s_affective_ctx = *ctx;
    } else {
        memset(&s_affective_ctx, 0, sizeof(s_affective_ctx));
    }
}

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
    s_current_soma.palpebral_aperture = 1.0f;
    s_current_soma.hardware_contrast = 180;
    s_current_soma.nystagmus_x = 0.0f;
    s_current_soma.nystagmus_y = 0.0f;
    memset(&s_affective_ctx, 0, sizeof(s_affective_ctx));
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
     * Driven dynamically by affective emotion (Valence/Arousal), homeostatic biological drives,
     * stereoscopic vergence, and autonomic vitality with zero rigid hardcoded constants. */
    float v_pos = relu_f(s_affective_ctx.valence);
    float v_neg = relu_f(-s_affective_ctx.valence);
    float fatigue = clamp_f(s_affective_ctx.fatigue, 0.0f, 1.0f);
    float curiosity = clamp_f(s_affective_ctx.curiosity, 0.0f, 1.0f);
    float arousal = clamp_f(s_affective_ctx.arousal, 0.0f, 1.0f);
    float boredom = clamp_f(s_affective_ctx.boredom, 0.0f, 1.0f);
    float mischief = clamp_f(s_affective_ctx.mischief, 0.0f, 1.0f);

    /* 5a. Vertical Height & Palpebral Squinting:
     * Duchenne smile narrows eyes from below (smiling crescent),
     * fatigue induces heavy-lidded drowsy squint,
     * high curiosity + arousal narrows eyes into focused scrutiny,
     * mischief triggers a subtle one-eyed sly wink/squint. */
    float squint_amount = 0.35f * v_pos + 0.36f * fatigue + 0.16f * (curiosity * arousal);
    float target_h_right = SOMA_CANONICAL_EYE_HEIGHT_PX * (1.0f - squint_amount);
    float wink_asym = (mischief > 0.35f) ? (0.20f * (mischief - 0.35f)) : 0.0f;
    float target_h_left = target_h_right * (1.0f - wink_asym);
    target_h_left = clamp_f(target_h_left, 18.0f, 38.0f);
    target_h_right = clamp_f(target_h_right, 18.0f, 38.0f);

    /* 5b. Horizontal Width & Volume-Conserving Tissue Dynamics:
     * Incompressibility expansion (w ~ 1 / sqrt(h)) balances squinting,
     * high arousal and curiosity expand width to absorb visual field. */
    float h_ratio = SOMA_CANONICAL_EYE_HEIGHT_PX / fmaxf(10.0f, target_h_right);
    float volume_compensation = sqrtf(h_ratio);
    float widen_factor = 1.0f + 0.18f * arousal + 0.10f * curiosity;
    float target_w = SOMA_CANONICAL_EYE_WIDTH_PX * widen_factor * (0.80f + 0.20f * volume_compensation);
    target_w = clamp_f(target_w, 24.0f, 42.0f);

    /* 5c. Superellipse Lamé Exponent n (Squircle Morphology):
     * Arousal softens corners into an innocent/surprised oval (n -> 2.2),
     * Boredom hardens corners into a flat, deadpan boxy squircle (n -> 5.6). */
    float target_n = SOMA_CANONICAL_SQUIRCLE_N - 1.8f * arousal + 1.4f * boredom;
    target_n = clamp_f(target_n, 2.0f, 5.8f);

    /* 5d. Orbital Tilt Orientation:
     * Strictly locked to 0.0 rad (level horizontal, upright) so the eyes
     * never appear crooked or tilted diagonally on the OLED. */
    float target_tilt_l = 0.0f;
    float target_tilt_r = 0.0f;

    /* 5e. Stereoscopic Depth Vergence:
     * Medial rectus inward convergence when focusing on near targets. */
    float v_clamped = clamp_f(s_affective_ctx.vergence_px, 0.0f, 3.5f);
    float target_left_x = SOMA_CANONICAL_LEFT_X + v_clamped;
    float target_right_x = SOMA_CANONICAL_RIGHT_X - v_clamped;

    /* 5f. Dynamic Stroke Thickness & OLED Contrast: */
    float stroke = SOMA_CANONICAL_STROKE_WIDTH;
    int raw_contrast = 40 + (int)(175.0f * s_metabolic_energy + 25.0f * y1);
    uint8_t hw_contrast = (uint8_t)clamp_f((float)raw_contrast, 30.0f, 255.0f);

    /* 5g. Low-Pass Exponential Smoothing (60 FPS Fluidity): */
    float alpha = 1.0f - expf(-8.0f * dt_sec);
    s_current_soma.left_w += (target_w - s_current_soma.left_w) * alpha;
    s_current_soma.right_w += (target_w - s_current_soma.right_w) * alpha;
    s_current_soma.left_h += (target_h_left - s_current_soma.left_h) * alpha;
    s_current_soma.right_h += (target_h_right - s_current_soma.right_h) * alpha;
    s_current_soma.left_n += (target_n - s_current_soma.left_n) * alpha;
    s_current_soma.right_n += (target_n - s_current_soma.right_n) * alpha;
    s_current_soma.tilt_left = target_tilt_l;
    s_current_soma.tilt_right = target_tilt_r;
    s_current_soma.left_x += (target_left_x - s_current_soma.left_x) * alpha;
    s_current_soma.right_x += (target_right_x - s_current_soma.right_x) * alpha;
    s_current_soma.stroke_thickness = stroke;
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
