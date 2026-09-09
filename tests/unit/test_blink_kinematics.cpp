/**
 * @file test_blink_kinematics.cpp
 * @brief Unit test suite for human eyelid kinematics, palpebral displacement, and tri-phase blink model.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <vector>

/* Standalone evaluation of kinematics functions for host unit testing */
static float eval_blink_close_ease(float t) {
    if (t <= 0.0f) return 1.0f;
    if (t >= 1.0f) return 0.0f;
    float u = std::pow(t, 0.85f);
    float s = u * u * u * (10.0f + u * (-15.0f + 6.0f * u));
    return std::clamp(1.0f - s, 0.0f, 1.0f);
}

static float eval_blink_open_ease(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float base = std::sin(t * 1.5707963f);
    float settling = 0.035f * std::sin(t * 3.14159265f) * std::exp(-2.5f * (1.0f - t));
    return std::clamp(base + settling, 0.0f, 1.04f);
}

struct PalpebralAperture {
    int y_top;
    int y_bottom;
    int height;
    int radius;
};

static PalpebralAperture compute_palpebral_aperture(float eyeHeightFactor, int oy = 0) {
    float h_clamped = std::clamp(eyeHeightFactor, 0.0f, 1.0f);
    float c = 1.0f - h_clamped;
    int y_top = (17 + oy) + (int)std::round(14.0f * std::pow(c, 0.75f));
    int y_bottom = (46 + oy) - (int)std::round(14.0f * std::pow(c, 1.50f));
    int curH = y_bottom - y_top + 1;
    if (curH < 2) curH = 2;
    if (curH > 30) curH = 30;
    int rad = (curH <= 2) ? 1 : ((curH < 8) ? 2 : ((curH < 16) ? 3 : ((curH < 24) ? 4 : 5)));
    return {y_top, y_bottom, curH, rad};
}

int main() {
    std::cout << "[TEST] Running human eyelid kinematics and palpebral displacement validation..." << std::endl;

    // Test 1: Boundary values for closing and opening
    assert(std::fabs(eval_blink_close_ease(0.0f) - 1.0f) < 1e-6f);
    assert(std::fabs(eval_blink_close_ease(1.0f) - 0.0f) < 1e-6f);
    assert(std::fabs(eval_blink_open_ease(0.0f) - 0.0f) < 1e-6f);
    assert(std::fabs(eval_blink_open_ease(1.0f) - 1.0f) < 1e-6f);
    std::cout << "[PASS] Boundary checks (0.0 and 1.0) verified." << std::endl;

    // Test 2: Closing phase monotonicity and velocity profile
    std::vector<float> close_vals;
    float prev_c = 1.0f;
    for (int i = 0; i <= 100; ++i) {
        float t = (float)i / 100.0f;
        float c = eval_blink_close_ease(t);
        assert(c <= prev_c + 1e-6f); // Strictly non-increasing
        assert(c >= 0.0f && c <= 1.0f);
        close_vals.push_back(c);
        prev_c = c;
    }

    // Measure peak closing velocity location
    int peak_vel_idx = 0;
    float max_vel = 0.0f;
    for (size_t i = 1; i + 1 < close_vals.size(); ++i) {
        float vel = (close_vals[i - 1] - close_vals[i + 1]) / 0.02f; // downward velocity
        if (vel > max_vel) {
            max_vel = vel;
            peak_vel_idx = (int)i;
        }
    }
    float peak_t = (float)peak_vel_idx / 100.0f;
    std::cout << "[INFO] Peak closing velocity at t = " << peak_t << ", max velocity = " << max_vel << std::endl;
    // Human orbicularis oculi peak velocity must occur in the initial 35% - 45% of trajectory
    assert(peak_t >= 0.35f && peak_t <= 0.45f);
    // Landing velocity at t = 1.0 must be cushioned (small)
    float landing_vel = (close_vals[98] - close_vals[100]) / 0.02f;
    assert(landing_vel < 0.25f);
    std::cout << "[PASS] Closing phase velocity profile matches human orbicularis fast-twitch kinematics." << std::endl;

    // Test 3: Opening phase continuity and gentle myogenic settling
    float prev_o = 0.0f;
    for (int i = 0; i <= 80; ++i) { // up to 80% opening is strictly increasing
        float t = (float)i / 100.0f;
        float o = eval_blink_open_ease(t);
        assert(o >= prev_o - 1e-6f);
        assert(o >= 0.0f && o <= 1.04f);
        prev_o = o;
    }
    float o_mid = eval_blink_open_ease(0.5f);
    assert(o_mid > 0.65f && o_mid < 0.78f); // Ease-out characteristic
    std::cout << "[PASS] Opening phase ease-out and aperture bounds verified." << std::endl;

    // Test 4: Biomechanical Asymmetric Palpebral Displacement geometry
    PalpebralAperture open_ap = compute_palpebral_aperture(1.0f);
    assert(open_ap.y_top == 17);
    assert(open_ap.y_bottom == 46);
    assert(open_ap.height == 30);
    assert(open_ap.radius == 5);

    PalpebralAperture closed_ap = compute_palpebral_aperture(0.0f);
    assert(closed_ap.y_top == 31);
    assert(closed_ap.y_bottom == 32);
    assert(closed_ap.height == 2);
    assert(closed_ap.radius == 1);

    // Upper eyelid dominance check at mid-blink (h = 0.5)
    PalpebralAperture mid_ap = compute_palpebral_aperture(0.5f);
    int upper_travel = mid_ap.y_top - open_ap.y_top; // downward distance
    int lower_travel = open_ap.y_bottom - mid_ap.y_bottom; // upward distance
    std::cout << "[INFO] At h=0.5: upper travel = " << upper_travel << " px, lower travel = " << lower_travel << " px" << std::endl;
    assert(upper_travel > lower_travel); // Upper eyelid must lead the blink
    assert(mid_ap.height > 2 && mid_ap.height < 30);

    // Drowsy slit check (h = 0.28, sleep micro-wake)
    PalpebralAperture drowsy_ap = compute_palpebral_aperture(0.28f);
    std::cout << "[INFO] Drowsy sleep micro-wake (h=0.28): height = " << drowsy_ap.height << " px, radius = " << drowsy_ap.radius << std::endl;
    assert(drowsy_ap.height >= 7 && drowsy_ap.height <= 10); // Realistic sleepy slit
    assert(drowsy_ap.radius >= 2);

    std::cout << "[SUCCESS] All human eyelid kinematics unit tests passed." << std::endl;
    return 0;
}
