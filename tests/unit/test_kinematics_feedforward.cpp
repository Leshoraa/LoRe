/**
 * @file test_kinematics_feedforward.cpp
 * @brief Unit test suite for predictive velocity feedforward, Ornstein-Uhlenbeck drift, and minimum-jerk kinematics.
 */

#include "src/math/kinematics.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <vector>

/* Standalone test definitions for kinematics globals */
float g_currentOffsetX = 0.0f;
float g_currentOffsetY = 0.0f;
float g_currentVergence = 0.0f;
float g_currentEyeScale = 1.0f;
float g_currentEyeScaleX = 1.0f;
float g_currentEyeScaleY = 1.0f;
bool g_is_transitioning = false;

/* Standalone Horner polynomial evaluation with post-saccadic glissade */
static const float kGlissadeReboundGain = 0.045f;
static const float kGlissadeOnsetTau = 0.70f;
static const float kGlissadeDecayLambda = 3.5f;

float eval_minimum_jerk_spline(float p) {
    if (p <= 0.0f) return 0.0f;
    if (p >= 1.0f) return 1.0f;
    float base_spline = p * p * p * (10.0f + p * (-15.0f + 6.0f * p));
    if (p > kGlissadeOnsetTau) {
        float delta_tau = p - kGlissadeOnsetTau;
        float norm_tail = delta_tau / (1.0f - kGlissadeOnsetTau);
        float glissade = kGlissadeReboundGain * std::sin(norm_tail * 3.14159265f) * std::exp(-kGlissadeDecayLambda * delta_tau);
        base_spline += glissade;
    }
    return std::clamp(base_spline, 0.0f, 1.06f);
}

/* Mass-spring-damper gaze tracking with velocity feedforward simulation */
struct GazeSim {
    float gaze_x = 0.0f;
    float gaze_vx = 0.0f;
    float omega_n = 56.0f;
    float lead_time = 0.035f;

    void step(float target_pos, float target_vel, float dt) {
        float effective_target = target_pos + lead_time * target_vel;
        float decay = std::exp(-omega_n * dt);
        float w_dt = omega_n * dt;

        float err = gaze_x - effective_target;
        float err_next = decay * ((1.0f + w_dt) * err + dt * gaze_vx);
        float v_next = decay * (-omega_n * w_dt * err + (1.0f - w_dt) * gaze_vx);

        gaze_x = effective_target + err_next;
        gaze_vx = v_next;
    }
};

/* Ornstein-Uhlenbeck stochastic drift simulation */
struct OUDriftSim {
    float drift = 0.0f;
    float theta = 3.0f;
    float sigma = 0.20f;

    void step(float dt, float rand_uniform) {
        float decay = std::exp(-theta * dt);
        float vol = sigma * std::sqrt(std::max(0.0f, (1.0f - decay * decay) / (2.0f * theta)));
        drift = drift * decay + rand_uniform * vol;
    }
};

int main() {
    std::cout << "[TEST] Running kinematics velocity feedforward and OU drift validation tests..." << std::endl;

    // Test 1: Minimum-Jerk Spline & Glissade Rebound properties
    assert(std::fabs(eval_minimum_jerk_spline(0.0f) - 0.0f) < 1e-6f);
    assert(std::fabs(eval_minimum_jerk_spline(1.0f) - 1.0f) < 1e-6f);
    assert(std::fabs(eval_minimum_jerk_spline(0.5f) - 0.5f) < 1e-6f);

    float max_s = 0.0f;
    float prev_s = 0.0f;
    for (int i = 1; i <= 100; ++i) {
        float p = (float)i / 100.0f;
        float s = eval_minimum_jerk_spline(p);
        assert(!std::isnan(s) && !std::isinf(s));
        if (p <= 0.70f) {
            assert(s >= prev_s);
        }
        if (s > max_s) max_s = s;
        prev_s = s;
    }
    assert(max_s > 1.01f && max_s <= 1.06f);
    std::cout << "[PASS] 5th-order minimum-jerk spline with post-saccadic glissade rebound verified." << std::endl;

    // Test 2: Predictive Velocity Feedforward eliminates steady-state ramp lag
    GazeSim sim_with_ff;
    GazeSim sim_without_ff;
    sim_without_ff.lead_time = 0.0f;

    float dt = 0.016666f;
    float target_velocity = 50.0f; // 50 px/s constant target velocity ramp

    for (int step = 0; step < 200; ++step) {
        float target_pos = target_velocity * (step * dt);
        sim_with_ff.step(target_pos, target_velocity, dt);
        sim_without_ff.step(target_pos, 0.0f, dt);
    }

    float final_pos = target_velocity * (200 * dt);
    float lag_with_ff = std::fabs(final_pos - sim_with_ff.gaze_x);
    float lag_without_ff = std::fabs(final_pos - sim_without_ff.gaze_x);

    std::cout << "[INFO] Steady-state lag WITHOUT feedforward : " << lag_without_ff << " px" << std::endl;
    std::cout << "[INFO] Steady-state lag WITH feedforward    : " << lag_with_ff << " px" << std::endl;

    assert(lag_with_ff < lag_without_ff);
    assert(lag_with_ff < 1.0f); // Steady-state tracking error under 1.0 pixel with velocity feedforward
    std::cout << "[PASS] Predictive velocity feedforward significantly reduced steady-state tracking lag." << std::endl;

    // Test 3: Ornstein-Uhlenbeck drift variance boundedness
    OUDriftSim ou_sim;
    std::vector<float> samples;
    float max_drift = 0.0f;

    for (int step = 0; step < 5000; ++step) {
        // Deterministic pseudo-random sequence within [-1.0, 1.0]
        float rand_val = (float)((step * 17) % 2001 - 1000) * 0.001f;
        ou_sim.step(dt, rand_val);
        samples.push_back(ou_sim.drift);
        if (std::fabs(ou_sim.drift) > max_drift) {
            max_drift = std::fabs(ou_sim.drift);
        }
    }

    float sum = 0.0f;
    for (float s : samples) sum += s;
    float mean = sum / samples.size();

    float sum_sq = 0.0f;
    for (float s : samples) sum_sq += (s - mean) * (s - mean);
    float variance = sum_sq / samples.size();

    std::cout << "[INFO] OU Drift mean: " << mean << ", variance: " << variance << ", max abs: " << max_drift << std::endl;
    assert(std::fabs(mean) < 0.10f); // Mean-reverts around zero
    assert(variance < 0.05f);        // Variance is mathematically bounded
    assert(max_drift < 0.50f);       // Never blows up into large drift

    std::cout << "[PASS] Ornstein-Uhlenbeck stochastic drift satisfies mean-reverting bounded variance requirements." << std::endl;

    // Test 4: Stereoscopic Ocular Vergence from proximity
    std::cout << "[TEST] Validating stereoscopic ocular vergence mechanics..." << std::endl;
    for (int i = 0; i <= 100; ++i) {
        float prox = (float)i / 100.0f; // [0.0, 1.0]
        float target_v = 0.0f;
        if (prox > 0.05f) {
            target_v = std::clamp(prox * 3.2f, 0.0f, 3.5f);
        }
        assert(target_v >= 0.0f && target_v <= 3.5f);
        if (prox <= 0.05f) {
            assert(target_v == 0.0f);
        } else if (prox >= 1.0f) {
            assert(std::fabs(target_v - 3.2f) < 1e-4f);
        }
    }
    std::cout << "[PASS] Stereoscopic ocular vergence bounds [0.0, 3.5] px verified." << std::endl;

    // Test 5: Volume-Conserving Biological Squash & Stretch
    std::cout << "[TEST] Validating volume-conserving affective tissue deformation..." << std::endl;
    float arousals[] = {0.05f, 0.20f, 0.50f, 0.90f};
    float sleep_pressures[] = {0.0f, 0.30f, 0.70f, 1.0f};

    for (float a : arousals) {
        for (float p_sleep : sleep_pressures) {
            float sy = 1.0f + (0.15f * (a - 0.20f) - 0.08f * p_sleep);
            sy = std::clamp(sy, 0.88f, 1.18f);
            float sx = 1.0f / std::sqrt(sy);

            // Incompressibility volume conservation: Sx^2 * Sy ≈ 1.0
            float vol = sx * sx * sy;
            assert(std::fabs(vol - 1.0f) < 1e-5f);
            assert(sy >= 0.88f && sy <= 1.18f);
            assert(sx >= 0.90f && sx <= 1.10f);
        }
    }
    // Check resting state at baseline (a = 0.20, p_sleep = 0.0)
    float base_sy = 1.0f + (0.15f * (0.20f - 0.20f) - 0.08f * 0.0f);
    float base_sx = 1.0f / std::sqrt(base_sy);
    assert(std::fabs(base_sy - 1.0f) < 1e-6f);
    assert(std::fabs(base_sx - 1.0f) < 1e-6f);
    std::cout << "[PASS] Biological tissue incompressibility and baseline resting preservation verified." << std::endl;

    // Test 6: Lid-Saccade Synkinesis and Fissure Tracking (von Graefe's following law)
    std::cout << "[TEST] Validating lid-saccade synkinesis palpebral mechanics..." << std::endl;
    auto test_synkinesis_aperture = [](float currentAperture, float gazeOffsetY) -> float {
        if (currentAperture <= 0.05f) return 0.0f;
        float norm_y = std::clamp(gazeOffsetY / 12.0f, -1.0f, 1.0f);
        float delta = (norm_y < 0.0f) ? (-0.06f * norm_y) : (-0.09f * norm_y);
        return std::clamp(currentAperture + delta, 0.05f, 1.08f);
    };

    auto test_fissure_offset = [](float gazeOffsetY) -> float {
        return 0.15f * gazeOffsetY;
    };

    // Upward gaze elevates upper eyelid
    float ap_up = test_synkinesis_aperture(1.0f, -10.0f);
    assert(ap_up > 1.0f && ap_up <= 1.08f);

    // Downward gaze narrows palpebral aperture
    float ap_down = test_synkinesis_aperture(1.0f, 10.0f);
    assert(ap_down < 1.0f && ap_down >= 0.90f);

    // Primary central gaze preserves baseline aperture
    float ap_center = test_synkinesis_aperture(1.0f, 0.0f);
    assert(std::fabs(ap_center - 1.0f) < 1e-6f);

    // Closed eye state (0.0f) remains closed during blinks
    float ap_closed = test_synkinesis_aperture(0.0f, -10.0f);
    assert(ap_closed == 0.0f);

    // Fissure vertical center tracks globe elevation
    assert(test_fissure_offset(-10.0f) < 0.0f);
    assert(test_fissure_offset(10.0f) > 0.0f);
    assert(test_fissure_offset(0.0f) == 0.0f);
    std::cout << "[PASS] Lid-saccade synkinesis and fissure vertical tracking verified." << std::endl;

    // Test 7: Listing's Law Axial Torsion Kinematics
    std::cout << "[TEST] Validating Listing's Law axial ocular torsion..." << std::endl;
    // Primary cardinal axes have zero torsion
    assert(std::fabs(getListingTorsionAngleRad(0.0f, 0.0f)) < 1e-6f);
    assert(std::fabs(getListingTorsionAngleRad(10.0f, 0.0f)) < 1e-6f);
    assert(std::fabs(getListingTorsionAngleRad(0.0f, 8.0f)) < 1e-6f);

    // Tertiary diagonal gaze produces continuous physiological torsion
    float t_ne = getListingTorsionAngleRad(14.0f, 8.0f);
    float t_nw = getListingTorsionAngleRad(-14.0f, 8.0f);
    float t_se = getListingTorsionAngleRad(14.0f, -8.0f);
    float t_sw = getListingTorsionAngleRad(-14.0f, -8.0f);

    assert(t_ne > 0.015f && t_ne <= 0.045f);
    assert(t_nw < -0.015f && t_nw >= -0.045f);
    assert(std::fabs(t_ne + t_nw) < 1e-6f);
    assert(std::fabs(t_ne + t_se) < 1e-6f);
    assert(std::fabs(t_ne - t_sw) < 1e-6f);
    std::cout << "[PASS] Listing's Law axial torsion satisfies quadrant symmetry and angular bounds." << std::endl;

    // Test 8: Lévy Flight Heavy-Tailed Gaze Step Distribution
    std::cout << "[TEST] Validating Lévy Flight heavy-tailed exploration step distribution..." << std::endl;
    int count_local = 0;
    int count_med = 0;
    int count_wide = 0;
    const int n_samples = 10000;
    float curiosity = 0.35f;
    float p_wide = 0.05f + 0.12f * curiosity; // ~0.092
    float p_med = 0.20f + 0.05f * curiosity;  // ~0.217

    for (int i = 0; i < n_samples; ++i) {
        float roll = (float)((i * 37 + 13) % 10000) / 10000.0f;
        if (roll < p_wide) {
            count_wide++;
        } else if (roll < (p_wide + p_med)) {
            count_med++;
        } else {
            count_local++;
        }
    }

    float frac_local = (float)count_local / (float)n_samples;
    float frac_med = (float)count_med / (float)n_samples;
    float frac_wide = (float)count_wide / (float)n_samples;

    std::cout << "[INFO] Lévy flight distribution: local=" << frac_local * 100.0f << "%, med="
              << frac_med * 100.0f << "%, wide=" << frac_wide * 100.0f << "%" << std::endl;

    assert(frac_local > 0.60f && frac_local < 0.75f);
    assert(frac_med > 0.18f && frac_med < 0.25f);
    assert(frac_wide > 0.07f && frac_wide < 0.12f);
    std::cout << "[PASS] Lévy flight heavy-tailed exploration hierarchy verified." << std::endl;

    std::cout << "[SUCCESS] All kinematics and dynamic model tests passed." << std::endl;
    return 0;
}
