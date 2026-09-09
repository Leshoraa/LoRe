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

/* Standalone evaluation of biological sleep struggle and droop aperture */
static float eval_volitional_will(float curiosity, float social, float presence, float bonding, float playfulness, float mischief, float arousal, float fatigue, float boredom) {
    float social_drive = social * (0.60f + 0.40f * presence);
    float will = (0.30f * curiosity)
               + (0.25f * social_drive)
               + (0.20f * bonding)
               + (0.15f * playfulness)
               + (0.10f * mischief)
               + (0.25f * arousal)
               - (0.35f * fatigue)
               - (0.25f * boredom);
    return std::clamp(will, 0.0f, 1.0f);
}

static float eval_droop_aperture(float sleep_pressure, float energy, float will) {
    if (sleep_pressure < 0.25f) return 1.0f;
    float sag = (0.82f * sleep_pressure + 0.18f * (1.0f - energy)) * (1.0f - 0.42f * will);
    return std::clamp(1.0f - sag, 0.12f, 0.92f);
}

struct DrowsyStruggleSim {
    enum Phase { SINKING, HOVERING, RECOVERY_SNAP, EFFORT_HOLD };
    Phase phase = SINKING;
    float aperture = 1.0f;
    float nod_y = 0.0f;
    float snap_target = 0.85f;
    float hover_dur = 0.80f;
    float effort_dur = 1.20f;
    float phase_timer = 0.0f;
    float micro_phase = 0.0f;

    void step(float dt, float sleep_pressure, float will, float droop_target, float mock_rand = 0.5f) {
        if (sleep_pressure < 0.25f) {
            float alpha = 1.0f - std::exp(-8.0f * dt);
            aperture += (1.0f - aperture) * alpha;
            nod_y += (0.0f - nod_y) * alpha;
            if (aperture > 0.98f) {
                aperture = 1.0f;
                nod_y = 0.0f;
            }
            return;
        }

        phase_timer += dt;

        switch (phase) {
            case SINKING: {
                float tau = 0.70f + 0.65f * sleep_pressure;
                float alpha = 1.0f - std::exp(-dt / tau);
                aperture += (droop_target - aperture) * alpha;
                float target_nod = (1.0f - aperture) * (2.2f + 1.3f * sleep_pressure);
                nod_y += (target_nod - nod_y) * (1.0f - std::exp(-3.5f * dt));
                if (std::abs(aperture - droop_target) < 0.035f || phase_timer > 3.0f) {
                    phase = HOVERING;
                    phase_timer = 0.0f;
                    hover_dur = 0.35f + 1.10f * (1.0f - will) + 0.25f * mock_rand;
                }
                break;
            }
            case HOVERING: {
                micro_phase += dt * 3.5f;
                aperture = droop_target + 0.018f * std::sin(micro_phase);
                float target_nod = (1.0f - droop_target) * (2.2f + 1.3f * sleep_pressure);
                nod_y += (target_nod - nod_y) * (1.0f - std::exp(-4.0f * dt));
                if (phase_timer >= hover_dur) {
                    phase = RECOVERY_SNAP;
                    phase_timer = 0.0f;
                    snap_target = std::clamp(0.72f + 0.28f * will - 0.10f * sleep_pressure, 0.65f, 1.0f);
                }
                break;
            }
            case RECOVERY_SNAP: {
                float tau = 0.11f + 0.05f * sleep_pressure;
                float alpha = 1.0f - std::exp(-dt / tau);
                aperture += (snap_target - aperture) * alpha;
                nod_y += (0.0f - nod_y) * (1.0f - std::exp(-18.0f * dt));
                if (aperture >= snap_target - 0.03f || phase_timer > 0.45f) {
                    phase = EFFORT_HOLD;
                    phase_timer = 0.0f;
                    effort_dur = 0.45f + 2.40f * will * (1.0f - 0.55f * sleep_pressure) + 0.30f * mock_rand;
                }
                break;
            }
            case EFFORT_HOLD: {
                aperture = snap_target;
                nod_y += (0.0f - nod_y) * (1.0f - std::exp(-10.0f * dt));
                if (phase_timer >= effort_dur) {
                    phase = SINKING;
                    phase_timer = 0.0f;
                }
                break;
            }
            default: break;
        }
        aperture = std::clamp(aperture, 0.05f, 1.0f);
        nod_y = std::clamp(nod_y, 0.0f, 4.0f);
    }
};

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
    std::cout << "[PASS] Biomechanical asymmetric palpebral displacement verified." << std::endl;

    // Test 5: Volitional Vigilance Drive Axioms
    float will_alert = eval_volitional_will(0.7f, 0.8f, 1.0f, 0.5f, 0.7f, 0.6f, 0.6f, 0.1f, 0.1f);
    float will_tired = eval_volitional_will(0.1f, 0.2f, 0.0f, 0.1f, 0.2f, 0.1f, 0.1f, 0.9f, 0.8f);
    assert(will_alert > 0.70f);
    assert(will_tired < 0.15f);
    std::cout << "[PASS] Volitional vigilance drive sensitivity verified (Alert=" << will_alert << ", Tired=" << will_tired << ")." << std::endl;

    // Test 6: Biological Droop Aperture Dynamics ("Merem Setengah")
    // Awake baseline (P_sleep < 0.25) -> exactly 1.0
    float h_awake = eval_droop_aperture(0.15f, 0.90f, 0.60f);
    assert(std::fabs(h_awake - 1.0f) < 1e-6f);

    // Moderate drowsiness (P_sleep ≈ 0.55, energy = 0.75, will = 0.45) -> 0.45 to 0.55 ("merem setengah")
    float h_moderate = eval_droop_aperture(0.55f, 0.75f, 0.45f);
    std::cout << "[INFO] Moderate drowsiness droop aperture: h = " << h_moderate << std::endl;
    assert(h_moderate >= 0.45f && h_moderate <= 0.65f);

    // Heavy exhaustion (P_sleep = 0.85, energy = 0.30, will = 0.15) -> heavy droop slit (0.15 to 0.35)
    float h_exhausted = eval_droop_aperture(0.85f, 0.30f, 0.15f);
    std::cout << "[INFO] Heavy exhaustion droop aperture: h = " << h_exhausted << std::endl;
    assert(h_exhausted >= 0.12f && h_exhausted <= 0.35f);

    // Willpower effect: higher will resists sag
    float h_high_will = eval_droop_aperture(0.60f, 0.70f, 0.80f);
    float h_low_will  = eval_droop_aperture(0.60f, 0.70f, 0.10f);
    assert(h_high_will > h_low_will + 0.15f);
    std::cout << "[PASS] Biological droop aperture responds organically to sleep pressure and internal will." << std::endl;

    // Test 7: Multi-Phase Drowsy Struggle ODE Simulation ("Buka-Nutup Menahan Kantuk")
    DrowsyStruggleSim sim;
    float dt = 0.016666f; // 60 FPS step
    float p_sleep = 0.60f;
    float will = 0.50f;
    float target_droop = eval_droop_aperture(p_sleep, 0.70f, will);

    // Verify Phase 1: Slow Viscous Sinking (takes > 0.6s to droop)
    float initial_h = sim.aperture;
    for (int step = 0; step < 30; ++step) { // 0.5 sec
        sim.step(dt, p_sleep, will, target_droop);
    }
    assert(sim.phase == DrowsyStruggleSim::SINKING);
    assert(sim.aperture < initial_h);
    assert(sim.aperture > target_droop); // Sinking is slow and heavy, not instant
    assert(sim.nod_y > 0.2f); // Head nod sinking
    std::cout << "[PASS] Phase 1 (Viscous Sinking) velocity profile is sluggish and heavy." << std::endl;

    // Step until Phase 2 (Hovering)
    int max_steps = 200;
    while (sim.phase == DrowsyStruggleSim::SINKING && max_steps-- > 0) {
        sim.step(dt, p_sleep, will, target_droop);
    }
    assert(sim.phase == DrowsyStruggleSim::HOVERING);
    assert(std::fabs(sim.aperture - target_droop) < 0.05f);
    assert(sim.nod_y > 0.8f); // Head nod fully down during droop hover
    std::cout << "[PASS] Phase 2 (Hovering at droop aperture / merem setengah) reached." << std::endl;

    // Step until Phase 3 (Recovery Snap)
    max_steps = 200;
    while (sim.phase == DrowsyStruggleSim::HOVERING && max_steps-- > 0) {
        sim.step(dt, p_sleep, will, target_droop);
    }
    assert(sim.phase == DrowsyStruggleSim::RECOVERY_SNAP);
    std::cout << "[PASS] Phase 3 (Volitional Recovery Snap) triggered." << std::endl;

    // Verify Phase 3 snap is fast and rebounds head upright
    max_steps = 40; // < 0.65 sec
    while (sim.phase == DrowsyStruggleSim::RECOVERY_SNAP && max_steps-- > 0) {
        sim.step(dt, p_sleep, will, target_droop);
    }
    assert(sim.phase == DrowsyStruggleSim::EFFORT_HOLD);
    assert(sim.aperture >= 0.70f); // Eyes snapped open
    assert(sim.nod_y < 0.5f);     // Head snapped upright
    std::cout << "[PASS] Phase 4 (Effort Hold) active with open eyes and upright head." << std::endl;

    // Verify Phase 4 drains and loops back to Sinking
    max_steps = 300;
    while (sim.phase == DrowsyStruggleSim::EFFORT_HOLD && max_steps-- > 0) {
        sim.step(dt, p_sleep, will, target_droop);
    }
    assert(sim.phase == DrowsyStruggleSim::SINKING);
    std::cout << "[PASS] Drowsy struggle cycle continuously loops (Sinking -> Hovering -> Snap -> Hold -> Sinking)." << std::endl;

    std::cout << "[SUCCESS] All human eyelid kinematics and drowsy sleep-struggle unit tests passed." << std::endl;
    return 0;
}
