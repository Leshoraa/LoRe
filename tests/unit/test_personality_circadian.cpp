/**
 * @file test_personality_circadian.cpp
 * @brief Host unit test suite for Personality Traits & Circadian Rhythm dynamics.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <cstdint>

struct PersonalitySim {
    float boldness = 0.55f;
    float volatility = 0.40f;
    float playfulness = 0.65f;
    float attachment = 0.50f;

    float energy_level = 0.60f;
    float mood_baseline = 0.0f;
    float activity_drive = 0.50f;
    float phase_pct = 0.0f;

    void updateCircadian(uint32_t elapsed_ms, uint32_t period_ms = 21600000) {
        float phase_raw = std::fmod((float)elapsed_ms, (float)period_ms) / (float)period_ms;
        phase_pct = phase_raw * 100.0f;

        float theta = 6.2831853f * (phase_raw - 0.375f);
        energy_level = std::max(0.20f, std::min(1.0f, 0.65f + 0.35f * std::cos(theta)));

        float mood_theta = 6.2831853f * (phase_raw - 0.30f);
        mood_baseline = std::max(-0.20f, std::min(0.25f, 0.025f + 0.20f * std::cos(mood_theta)));

        float act_theta = 6.2831853f * (phase_raw - 0.30f);
        activity_drive = std::max(0.10f, std::min(1.0f, 0.55f + 0.40f * std::cos(act_theta)));
    }

    float getValenceSigma() const {
        float base = 0.012f + 0.020f * volatility;
        return base * (0.70f + 0.30f * energy_level);
    }

    float getGazeOmega() const {
        float base = 32.0f;
        float bold_factor = 0.85f + 0.30f * boldness;
        float energy_factor = 0.80f + 0.20f * energy_level;
        return base * bold_factor * energy_factor;
    }

    float getGazeDamping() const {
        return 0.90f - 0.28f * boldness;
    }

    float getIdleYBias() const {
        float shyness = 1.0f - boldness;
        float rest_factor = 1.0f + 0.5f * (1.0f - energy_level);
        return shyness * 2.5f * rest_factor;
    }

    float getSocialGain() const {
        return 0.15f + 0.20f * attachment;
    }

    float getMischiefGain() const {
        return 0.08f + 0.14f * playfulness;
    }

    float getAmbientGlanceScale() const {
        float curiosity_factor = 1.0f - 0.30f * playfulness;
        float activity_factor = 1.0f - 0.20f * activity_drive;
        return std::max(0.40f, std::min(1.20f, curiosity_factor * activity_factor));
    }
};

int main() {
    std::cout << "[TEST] Running Personality Trait & Circadian Rhythm validation suite..." << std::endl;

    PersonalitySim sim;

    // Test 1: Circadian Phase Evolution across 24 hours (4 full 6-hour cycles)
    uint32_t period = 21600000;
    for (uint32_t t = 0; t <= period * 4; t += 60000) { // step by 1 minute
        sim.updateCircadian(t, period);
        assert(sim.energy_level >= 0.20f && sim.energy_level <= 1.0f);
        assert(sim.mood_baseline >= -0.20f && sim.mood_baseline <= 0.25f);
        assert(sim.activity_drive >= 0.10f && sim.activity_drive <= 1.0f);
        assert(sim.phase_pct >= 0.0f && sim.phase_pct <= 100.0f);
        assert(!std::isnan(sim.energy_level) && !std::isnan(sim.mood_baseline));
    }
    std::cout << "[PASS] Circadian cycle smoothly oscillates within bounded physiological limits." << std::endl;

    // Test 2: Energy peaks at ~37.5% phase and troughs at ~87.5% phase
    sim.updateCircadian((uint32_t)(period * 0.375f), period);
    assert(sim.energy_level > 0.95f); // Near peak
    sim.updateCircadian((uint32_t)(period * 0.875f), period);
    assert(sim.energy_level < 0.35f); // Near trough
    std::cout << "[PASS] Peak alertness and rest phase timing verified." << std::endl;

    // Test 3: Boldness modulation on Gaze Kinematics
    sim.boldness = 1.0f; // Maximum bold
    float omega_bold = sim.getGazeOmega();
    float zeta_bold = sim.getGazeDamping();
    float y_bias_bold = sim.getIdleYBias();

    sim.boldness = 0.0f; // Maximum shy
    float omega_shy = sim.getGazeOmega();
    float zeta_shy = sim.getGazeDamping();
    float y_bias_shy = sim.getIdleYBias();

    assert(omega_bold > omega_shy); // Bold produces faster natural frequency
    assert(zeta_bold < zeta_shy);   // Bold is snappy/underdamped, shy is sluggish/overdamped
    assert(y_bias_shy > y_bias_bold); // Shy looks down significantly more
    std::cout << "[PASS] Boldness trait modulation on gaze kinematics verified." << std::endl;

    // Test 4: Volatility and Langevin Sigma
    sim.volatility = 0.0f;
    float sigma_stable = sim.getValenceSigma();
    sim.volatility = 1.0f;
    float sigma_volatile = sim.getValenceSigma();
    assert(sigma_volatile > sigma_stable * 2.0f);
    std::cout << "[PASS] Volatility modulation on Langevin diffusion sigma verified." << std::endl;

    // Test 5: Ambient Glance Scaling
    sim.playfulness = 1.0f;
    sim.activity_drive = 1.0f;
    float scale_active = sim.getAmbientGlanceScale();
    sim.playfulness = 0.0f;
    sim.activity_drive = 0.1f;
    float scale_stoic = sim.getAmbientGlanceScale();
    assert(scale_active < scale_stoic); // Active/playful checks clock/weather more frequently (shorter interval)
    std::cout << "[PASS] Ambient glance frequency scaling verified." << std::endl;

    // Test 6: Astronomical Sunrise / Sunset & Circadian Rhythm Alignment
    uint8_t sunrise_h = 5, sunrise_m = 49;  // 05:49 AM
    uint8_t sunset_h = 17, sunset_m = 51;  // 17:51 PM
    int sunrise_min = sunrise_h * 60 + sunrise_m; // 349 min
    int sunset_min = sunset_h * 60 + sunset_m;   // 1071 min

    auto eval24h = [&](int hour, int min, bool& out_sleep, bool& out_deep_sleep, bool& out_wake, float& out_drowsy) {
        int now_min = hour * 60 + min;
        int deep_start = 1 * 60 + 0;   // 01:00
        int deep_end = 5 * 60 + 30;    // 05:30
        if (sunrise_min < deep_end) deep_end = sunrise_min;

        out_deep_sleep = (now_min >= deep_start && now_min < deep_end);
        out_sleep = (now_min >= 23 * 60 || now_min < sunrise_min);
        out_wake = (now_min >= sunrise_min && now_min < sunrise_min + 75);

        if (now_min >= 23 * 60) {
            out_drowsy = 0.70f + 0.25f * ((float)min / 60.0f);
        } else if (now_min < sunrise_min) {
            out_drowsy = 0.95f;
        } else if (out_wake) {
            float progress = (float)(now_min - sunrise_min) / 75.0f;
            out_drowsy = 0.55f * (1.0f - progress);
        } else if (now_min >= sunset_min - 30) {
            int span = (23 * 60) - (sunset_min - 30);
            float progress = (float)(now_min - (sunset_min - 30)) / (float)span;
            out_drowsy = 0.65f * progress;
        } else {
            out_drowsy = 0.0f;
        }
    };

    bool is_sleep, is_deep_sleep, is_wake;
    float drowsy;

    // Midday (12:00)
    eval24h(12, 0, is_sleep, is_deep_sleep, is_wake, drowsy);
    assert(!is_sleep && !is_deep_sleep && !is_wake && drowsy == 0.0f);

    // Dusk (18:30, after sunset)
    eval24h(18, 30, is_sleep, is_deep_sleep, is_wake, drowsy);
    assert(!is_sleep && !is_deep_sleep && !is_wake && drowsy > 0.0f && drowsy < 0.40f);

    // Late night bedtime (23:15)
    eval24h(23, 15, is_sleep, is_deep_sleep, is_wake, drowsy);
    assert(is_sleep && !is_deep_sleep && drowsy >= 0.70f);

    // Deep night (02:30, OLED Deep Sleep Window)
    eval24h(2, 30, is_sleep, is_deep_sleep, is_wake, drowsy);
    assert(is_sleep && is_deep_sleep && drowsy == 0.95f);

    // Sunrise dawn awakening (05:49)
    eval24h(5, 49, is_sleep, is_deep_sleep, is_wake, drowsy);
    assert(!is_sleep && !is_deep_sleep && is_wake && std::abs(drowsy - 0.55f) < 0.01f);

    // Morning wake transition progress (06:20)
    eval24h(6, 20, is_sleep, is_deep_sleep, is_wake, drowsy);
    assert(!is_sleep && !is_deep_sleep && is_wake && drowsy < 0.40f && drowsy > 0.10f);

    // Fully awake morning (07:30)
    eval24h(7, 30, is_sleep, is_deep_sleep, is_wake, drowsy);
    assert(!is_sleep && !is_deep_sleep && !is_wake && drowsy == 0.0f);
    std::cout << "[PASS] Astronomical sunrise/sunset circadian & deep sleep validation verified." << std::endl;

    std::cout << "[SUCCESS] All Personality and Circadian Rhythm tests passed!" << std::endl;
    return 0;
}
