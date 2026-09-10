/**
 * @file test_sleep_gated_restlessness.cpp
 * @brief Algorithmic unit test suite for First-Principles Sleep-Gated Restlessness and Spontaneous Play.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>

struct RestlessnessModel {
    static float computeSleepGate(float sleep_pressure) {
        float awake_vitality = std::max(0.0f, 1.0f - sleep_pressure);
        return awake_vitality * awake_vitality;
    }

    static void computeSolitudeModulation(float sleep_pressure, float boredom, float mischief, float curiosity,
                                         float& delta_a, float& delta_v) {
        float sleep_gate = computeSleepGate(sleep_pressure);
        delta_a = (0.14f * boredom * mischief + 0.06f * curiosity) * sleep_gate;
        delta_v = (0.08f * (curiosity - 0.20f) + 0.04f * mischief) * sleep_gate;
    }

    static float computeLevyWideProb(float base_prob, float curiosity_gain, float curiosity,
                                     float boredom, float sleep_pressure) {
        float sleep_gate = computeSleepGate(sleep_pressure);
        return base_prob + curiosity_gain * curiosity + 0.08f * boredom * sleep_gate;
    }
};

int main() {
    std::cout << "[TEST] Running Sleep-Gated Restlessness & Spontaneous Play mathematical validation..." << std::endl;

    // Test 1: Active, well-rested solitude produces significant restlessness and play drive
    {
        float sleep_pressure = 0.05f; // fresh, wide awake
        float boredom = 0.85f;        // alone on desk for a while
        float mischief = 0.70f;       // playful disposition
        float curiosity = 0.65f;

        float delta_a = 0.0f, delta_v = 0.0f;
        RestlessnessModel::computeSolitudeModulation(sleep_pressure, boredom, mischief, curiosity, delta_a, delta_v);

        std::cout << "[INFO] Low sleep pressure (0.05): delta_a = " << delta_a << ", delta_v = " << delta_v << std::endl;
        assert(delta_a > 0.07f && delta_a < 0.20f);
        assert(delta_v > 0.02f && delta_v < 0.10f);

        float p_wide = RestlessnessModel::computeLevyWideProb(0.05f, 0.12f, curiosity, boredom, sleep_pressure);
        std::cout << "[INFO] Wide exploratory gaze saccade probability: " << p_wide << " (base 0.05)" << std::endl;
        assert(p_wide > 0.15f); // Increased peripheral inspection when bored and awake
        std::cout << "[PASS] Well-rested boredom generates natural endogenous exploration drive." << std::endl;
    }

    // Test 2: High sleep debt (exhaustion/night) completely suppresses restlessness
    {
        float sleep_pressure = 0.88f; // heavy sleep pressure (Process S)
        float boredom = 0.85f;
        float mischief = 0.70f;
        float curiosity = 0.65f;

        float delta_a = 0.0f, delta_v = 0.0f;
        RestlessnessModel::computeSolitudeModulation(sleep_pressure, boredom, mischief, curiosity, delta_a, delta_v);

        std::cout << "[INFO] High sleep pressure (0.88): delta_a = " << delta_a << ", delta_v = " << delta_v << std::endl;
        // Sleep gate = (1 - 0.88)^2 = 0.0144 -> delta_a must be negligible (< 0.003)
        assert(delta_a < 0.003f);
        assert(std::fabs(delta_v) < 0.002f);

        float p_wide = RestlessnessModel::computeLevyWideProb(0.05f, 0.12f, curiosity, boredom, sleep_pressure);
        std::cout << "[INFO] Drowsy wide exploratory probability: " << p_wide << std::endl;
        // Restlessness contribution is ~0.08 * 0.85 * 0.0144 = 0.00098 -> p_wide stays low
        assert(p_wide < 0.14f);
        std::cout << "[PASS] Biological sleep pressure strictly inhibits restlessness, allowing natural droop and sleep." << std::endl;
    }

    // Test 3: Monotonicity and continuity across full sleep pressure spectrum [0.0, 1.0]
    {
        float prev_gate = 1.1f;
        for (int p_int = 0; p_int <= 100; ++p_int) {
            float p = (float)p_int * 0.01f;
            float gate = RestlessnessModel::computeSleepGate(p);
            assert(!std::isnan(gate) && !std::isinf(gate));
            assert(gate >= 0.0f && gate <= 1.0f);
            assert(gate <= prev_gate + 1e-6f); // Strictly non-increasing
            prev_gate = gate;
        }
        assert(RestlessnessModel::computeSleepGate(1.0f) == 0.0f);
        std::cout << "[PASS] Continuous C1 monotonicity verified across [0, 1]." << std::endl;
    }

    // Test 4: Extreme Drive Space Boundary Safety
    {
        for (float p : {0.0f, 0.5f, 1.0f}) {
            for (float b : {0.0f, 0.5f, 1.0f}) {
                for (float m : {0.0f, 0.5f, 1.0f}) {
                    for (float c : {0.0f, 0.5f, 1.0f}) {
                        float da = 0.0f, dv = 0.0f;
                        RestlessnessModel::computeSolitudeModulation(p, b, m, c, da, dv);
                        assert(!std::isnan(da) && !std::isinf(da));
                        assert(!std::isnan(dv) && !std::isinf(dv));
                        assert(da >= 0.0f && da <= 0.30f);
                        assert(dv >= -0.10f && dv <= 0.20f);
                    }
                }
            }
        }
        std::cout << "[PASS] Extreme drive space bounds and numerical stability verified." << std::endl;
    }

    std::cout << "[SUCCESS] All sleep-gated restlessness and play validation tests passed!" << std::endl;
    return 0;
}
