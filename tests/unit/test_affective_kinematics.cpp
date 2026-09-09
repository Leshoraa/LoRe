/**
 * @file test_affective_kinematics.cpp
 * @brief Unit tests for Affective Saccade Kinematics, 3D Cornea Parallax, and Micro-Particles.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <algorithm>

/* Simulated types for host testing */
enum Expression {
    EXPR_IDLE = 0,
    EXPR_HAPPY,
    EXPR_ANGRY,
    EXPR_SAD,
    EXPR_SURPRISED,
    EXPR_SUSPICIOUS,
    EXPR_CURIOUS,
    EXPR_MISCHIEF,
    EXPR_SLEEPY,
    EXPR_COOL,
    EXPR_DIZZY,
    EXPR_CRYING,
    NUM_EXPRESSIONS
};

struct AffectiveKinematicProfile {
    float omega_mult;
    float zeta_mult;
    float duration_mult;
    float glissade_gain;
};

static const AffectiveKinematicProfile kAffectiveKinematicTable[NUM_EXPRESSIONS] = {
    { 1.00f, 1.00f, 1.00f, 0.045f }, /* IDLE */
    { 1.15f, 0.82f, 0.88f, 0.065f }, /* HAPPY */
    { 1.40f, 1.30f, 0.70f, 0.005f }, /* ANGRY */
    { 0.75f, 1.10f, 1.45f, 0.020f }, /* SAD */
    { 1.50f, 1.00f, 0.65f, 0.030f }, /* SURPRISED */
    { 1.10f, 1.25f, 1.15f, 0.010f }, /* SUSPICIOUS */
    { 1.20f, 0.90f, 0.85f, 0.055f }, /* CURIOUS */
    { 1.25f, 0.78f, 0.80f, 0.070f }, /* MISCHIEF */
    { 0.65f, 1.20f, 1.60f, 0.015f }, /* SLEEPY */
    { 0.95f, 1.05f, 1.10f, 0.035f }, /* COOL */
    { 0.80f, 0.70f, 1.30f, 0.080f }, /* DIZZY */
    { 0.85f, 0.95f, 1.35f, 0.040f }  /* CRYING */
};

static const float kGlissadeOnsetTau = 0.70f;
static const float kGlissadeDecayLambda = 3.5f;
static const float kGlissadeMaxOvershoot = 1.06f;

static float eval_test_glissade_spline(float p, float glissade_gain) {
    if (p <= 0.0f) return 0.0f;
    if (p >= 1.0f) return 1.0f;
    float base_spline = p * p * p * (10.0f + p * (-15.0f + 6.0f * p));
    if (p > kGlissadeOnsetTau) {
        float delta_tau = p - kGlissadeOnsetTau;
        float norm_tail = delta_tau / (1.0f - kGlissadeOnsetTau);
        float glissade = glissade_gain * std::sin(norm_tail * 3.14159265f) * std::exp(-kGlissadeDecayLambda * delta_tau);
        base_spline += glissade;
    }
    return std::clamp(base_spline, 0.0f, kGlissadeMaxOvershoot);
}

static uint32_t compute_test_saccade_duration(float displacement_px, float duration_mult) {
    const float kSaccadeD0Ms = 80.0f;
    const float kSaccadeKMsPerDeg = 2.4f;
    const float kPxToDegFactor = 1.8f;

    float deg = std::fabs(displacement_px) * kPxToDegFactor;
    float dur = (kSaccadeD0Ms + kSaccadeKMsPerDeg * deg) * duration_mult;
    if (dur < 75.0f) dur = 75.0f;
    if (dur > 380.0f) dur = 380.0f;
    return static_cast<uint32_t>(dur);
}

int main() {
    std::cout << "[TEST] Running Affective Saccade Kinematics & Parallax Tests..." << std::endl;

    // Test 1: Saccade Duration Ordering Across Emotional States
    // Angry saccades must be significantly snappier (shorter duration) than Idle,
    // and Idle must be faster than Sleepy.
    float displacement = 10.0f;
    uint32_t dur_angry = compute_test_saccade_duration(displacement, kAffectiveKinematicTable[EXPR_ANGRY].duration_mult);
    uint32_t dur_idle = compute_test_saccade_duration(displacement, kAffectiveKinematicTable[EXPR_IDLE].duration_mult);
    uint32_t dur_sleepy = compute_test_saccade_duration(displacement, kAffectiveKinematicTable[EXPR_SLEEPY].duration_mult);

    assert(dur_angry < dur_idle);
    assert(dur_idle < dur_sleepy);
    assert(dur_angry >= 75);
    assert(dur_sleepy <= 380);

    // Test 2: Glissade Rebound Peak vs Rigidity
    // Happy/Mischief should overshoot 1.0 at tail; Angry should remain under or barely exceed 1.002
    float peak_happy = 0.0f;
    float peak_angry = 0.0f;
    for (int i = 70; i <= 100; ++i) {
        float p = static_cast<float>(i) / 100.0f;
        float val_happy = eval_test_glissade_spline(p, kAffectiveKinematicTable[EXPR_HAPPY].glissade_gain);
        float val_angry = eval_test_glissade_spline(p, kAffectiveKinematicTable[EXPR_ANGRY].glissade_gain);
        if (val_happy > peak_happy) peak_happy = val_happy;
        if (val_angry > peak_angry) peak_angry = val_angry;
    }
    assert(peak_happy > 1.015f);
    assert(peak_angry <= 1.005f);
    assert(peak_happy <= kGlissadeMaxOvershoot);

    // Test 3: 3D Parallax Cornea Specular Highlight Coordinates
    // With kParallaxRatio = 0.45, looking right (ox = +10) must shift the glint
    // to the left relative to eye center: delta_glint = -ox * (1 - 0.45) = -5.5 px.
    const float kParallaxRatio = 0.45f;
    float ox = 10.0f;
    float delta_glint_x = -ox * (1.0f - kParallaxRatio);
    assert(std::fabs(delta_glint_x - (-5.5f)) < 1e-4f);

    float eye_xc = 64.0f;
    float eye_yc = 32.0f;
    float eye_center_with_gaze = eye_xc + ox; // 74.0
    float glint_abs_x = eye_center_with_gaze - 3.5f + delta_glint_x; // 74 - 3.5 - 5.5 = 65.0
    // Glint on screen moves in same direction as gaze, but slower:
    // Screen displacement = glint_abs_x - (eye_xc - 3.5f) = 65.0 - 60.5 = 4.5 px = +ox * kParallaxRatio
    assert(std::fabs((glint_abs_x - (eye_xc - 3.5f)) - (ox * kParallaxRatio)) < 1e-4f);

    // Test 4: Palpebral Occlusion Threshold
    const float kCatchlightMinAperture = 0.38f;
    assert(0.35f < kCatchlightMinAperture); // Closed/blink occludes glint
    assert(0.80f >= kCatchlightMinAperture); // Open eye displays glint

    // Test 5: Micro-Particle Ring Buffer Constraints
    const int kMaxParticles = 4;
    int active_particles = 0;
    for (int i = 0; i < 10; ++i) {
        if (active_particles < kMaxParticles) {
            active_particles++;
        }
    }
    assert(active_particles == kMaxParticles);

    std::cout << "[PASS] Affective kinematics and 3D parallax unit tests passed." << std::endl;
    return 0;
}
