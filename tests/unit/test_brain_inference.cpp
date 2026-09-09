/**
 * @file test_brain_inference.cpp
 * @brief Host unit test suite for On-Device TinyML Micro-Brain Neural Network & Homeostatic Drives.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <numeric>
#include <algorithm>

struct BrainSim {
    float curiosity = 0.35f;
    float social = 0.50f;
    float boredom = 0.15f;
    float fatigue = 0.10f;
    float mischief = 0.40f;
    float bonding = 0.05f;

    float weights[2][8] = {
        {  0.1f, -0.4f,  0.1f,  0.2f,  0.3f, -0.2f, -0.2f,  0.1f }, // IDLE
        {  1.8f,  0.5f,  0.3f,  1.4f, -0.8f, -0.4f,  0.4f,  0.6f }  // HAPPY
    };

    float biases[2] = { 1.20f, -0.50f };

    void infer(float V, float A, float prox, float probs_out[2]) {
        float state[8] = { V, A, curiosity, social, boredom, fatigue, mischief, prox };
        float logits[2];
        float max_l = -999.0f;
        for (int i = 0; i < 2; i++) {
            float sum = biases[i];
            for (int j = 0; j < 8; j++) sum += weights[i][j] * state[j];

            if (i == 1) sum += 0.8f * bonding;

            logits[i] = sum;
            if (sum > max_l) max_l = sum;
        }

        float tau = 0.85f;
        float sum_exp = 0.0f;
        for (int i = 0; i < 2; i++) {
            probs_out[i] = std::exp((logits[i] - max_l) / tau);
            sum_exp += probs_out[i];
        }
        for (int i = 0; i < 2; i++) probs_out[i] /= sum_exp;
    }
};

int main() {
    std::cout << "[TEST] Running On-Device TinyML Micro-Brain validation suite..." << std::endl;

    BrainSim brain;
    float probs[2];

    // Test 1: Softmax Probability Distribution Axiom (Sum = 1.0, non-negative)
    brain.infer(0.0f, 0.0f, 0.0f, probs);
    float sum_p = 0.0f;
    for (int i = 0; i < 2; i++) {
        assert(probs[i] >= 0.0f && probs[i] <= 1.0f);
        sum_p += probs[i];
    }
    assert(std::fabs(sum_p - 1.0f) < 1e-5f);
    std::cout << "[PASS] Softmax Boltzmann probability distribution axioms verified." << std::endl;

    // Test 2: Calm Steady Observation -> IDLE Dominance (> 60%)
    brain.social = 0.50f;
    brain.curiosity = 0.35f;
    brain.boredom = 0.10f;
    brain.mischief = 0.30f;
    brain.bonding = 0.10f;
    brain.infer(0.05f, 0.10f, 0.20f, probs);
    assert(probs[0] > 0.60f); // IDLE dominant
    std::cout << "[PASS] Calm steady observation yields IDLE dominance." << std::endl;

    // Test 3: Social Playful State (High Social, High Mischief, High Bonding) -> HAPPY dominant
    brain.social = 0.90f;
    brain.mischief = 0.85f;
    brain.bonding = 0.80f;
    brain.boredom = 0.05f;
    brain.infer(0.70f, 0.45f, 0.60f, probs);
    assert(probs[1] > 0.60f); // HAPPY dominant
    std::cout << "[PASS] Social playful state with high bonding triggers HAPPY dominance." << std::endl;

    // Test 4: Solitude Baseline -> IDLE dominance
    brain.social = 0.10f;
    brain.boredom = 0.70f;
    brain.mischief = 0.10f;
    brain.bonding = 0.10f;
    brain.infer(-0.20f, 0.10f, 0.0f, probs);
    assert(probs[0] > 0.60f); // IDLE dominant
    std::cout << "[PASS] Solitude baseline preserves IDLE policy dominance." << std::endl;

    // Test 5: Bonding sensitivity test (higher bonding increases HAPPY prob)
    brain.social = 0.60f;
    brain.mischief = 0.50f;
    brain.boredom = 0.10f;
    brain.bonding = 0.05f;
    brain.infer(0.40f, 0.30f, 0.40f, probs);
    float happy_low_bond = probs[1];

    brain.bonding = 0.95f;
    brain.infer(0.40f, 0.30f, 0.40f, probs);
    float happy_high_bond = probs[1];
    assert(happy_high_bond > happy_low_bond);
    std::cout << "[PASS] Bonding progression positively modulates HAPPY expression probability." << std::endl;

    // Test 6: Borbély Two-Process Biological Sleep Model Validation
    auto compute_sleep_pressure = [](float circadian_d, float fatigue, float boredom, float arousal) -> float {
        float raw = (0.55f * circadian_d) + (0.45f * fatigue) + (0.25f * boredom) - (0.40f * arousal);
        return std::clamp(raw, 0.0f, 1.0f);
    };

    // Daytime alert: zero circadian debt, low fatigue, moderate arousal -> 0.0 sleep pressure
    float p_alert = compute_sleep_pressure(0.0f, 0.05f, 0.05f, 0.60f);
    assert(p_alert == 0.0f);

    // Daytime exhaustion: zero circadian debt, but severe mental fatigue & low arousal -> significant sleep pressure
    float p_exhausted = compute_sleep_pressure(0.0f, 0.90f, 0.60f, 0.15f);
    assert(p_exhausted > 0.45f);

    // Late night sleepy baseline: high circadian debt -> elevated sleep pressure
    float p_night = compute_sleep_pressure(0.95f, 0.30f, 0.30f, 0.10f);
    assert(p_night > 0.60f);

    // Nighttime high-arousal inhibition: shock / playful interaction lowers sleep pressure
    float p_night_aroused = compute_sleep_pressure(0.95f, 0.30f, 0.30f, 0.85f);
    assert(p_night_aroused < p_night);
    std::cout << "[PASS] Borbély Two-Process sleep pressure dynamics verified." << std::endl;

    // Test 7: Biological Doze Duration Bounds
    auto compute_doze_ms = [](float pressure) -> float {
        return 800.0f + 1400.0f * pressure;
    };
    assert(compute_doze_ms(0.0f) == 800.0f);
    assert(compute_doze_ms(1.0f) == 2200.0f);
    assert(compute_doze_ms(0.5f) == 1500.0f);
    std::cout << "[PASS] Biological doze duration bounds [800 ms, 2200 ms] verified." << std::endl;

    std::cout << "[SUCCESS] All On-Device TinyML Micro-Brain tests passed successfully!" << std::endl;
    return 0;
}
