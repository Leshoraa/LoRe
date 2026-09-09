/**
 * @file test_autonomic_cpg.cpp
 * @brief Host unit test suite for Autonomic Engine, Matsuoka CPG, and Ocular Soma dynamics.
 */

#include "src/ai/autonomic_engine.h"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
    std::cout << "[TEST] Running Autonomic Engine & Matsuoka CPG validation tests..." << std::endl;

    initAutonomicEngine();

    AutonomicTelemetry t_init = getAutonomicTelemetry();
    assert(t_init.metabolic_energy > 0.5f);
    assert(t_init.motor_tension >= 0.0f);

    float dt = 0.0166f; /* 60 FPS */
    float min_resp = 100.0f;
    float max_resp = -100.0f;
    bool has_spontaneous_trigger = false;

    /* Run 1200 steps (~20 seconds of biological simulation) */
    for (int step = 0; step < 1200; ++step) {
        updateAutonomicEngine(dt);

        AutonomicTelemetry t = getAutonomicTelemetry();
        OcularSomaState soma = getOcularSomaState();

        /* Verify numerical stability */
        assert(!std::isnan(t.respiration_phase));
        assert(!std::isinf(t.respiration_phase));
        assert(!std::isnan(t.metabolic_energy));
        assert(!std::isnan(t.motor_tension));

        /* Verify bounds */
        assert(t.metabolic_energy >= 0.10f && t.metabolic_energy <= 1.0f);
        assert(t.motor_tension >= 0.0f && t.motor_tension <= 1.0f);

        /* Verify soma bounds */
        assert(soma.left_w >= 20.0f && soma.left_w <= 45.0f);
        assert(soma.right_w >= 20.0f && soma.right_w <= 45.0f);
        assert(soma.left_h >= 18.0f && soma.left_h <= 40.0f);
        assert(soma.right_h >= 18.0f && soma.right_h <= 40.0f);
        assert(soma.left_n >= 1.5f && soma.left_n <= 6.0f);
        assert(soma.right_n >= 1.5f && soma.right_n <= 6.0f);
        assert(soma.hardware_contrast >= 25);
        assert(std::fabs(soma.nystagmus_x) <= 1.0f);
        assert(std::fabs(soma.nystagmus_y) <= 1.0f);

        if (t.respiration_phase < min_resp) min_resp = t.respiration_phase;
        if (t.respiration_phase > max_resp) max_resp = t.respiration_phase;

        if (consumeSpontaneousBlinkTrigger()) {
            has_spontaneous_trigger = true;
        }

        /* Periodic external stimulation */
        if (step == 400) {
            stimulateAutonomicEngine(0.6f);
        }
    }

    /* Assert that CPG actually produced a limit cycle oscillation */
    assert(max_resp > 0.05f);
    assert(min_resp < 0.0f);
    std::cout << "[PASS] Matsuoka CPG oscillation verified: resp_range = [" << min_resp << ", " << max_resp << "]" << std::endl;

    /* Assert that motor tension accumulation discharged at least once */
    assert(has_spontaneous_trigger);
    std::cout << "[PASS] Spontaneous autonomic volition / tension discharge verified." << std::endl;

    /* Test 2: High Valence Duchenne smile horizontal squinting */
    AutonomicAffectiveContext happy_ctx = {};
    happy_ctx.valence = 0.85f;
    happy_ctx.arousal = 0.50f;
    setAutonomicAffectiveContext(&happy_ctx);
    for (int i = 0; i < 60; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_happy = getOcularSomaState();
    assert(soma_happy.left_h < 30.0f);
    assert(std::fabs(soma_happy.tilt_left) < 1e-4f);
    assert(std::fabs(soma_happy.tilt_right) < 1e-4f);
    std::cout << "[PASS] Affective Duchenne smile horizontal squint verified: h = " << soma_happy.left_h << ", tilt = 0.0" << std::endl;

    /* Test 3: Fatigue-driven palpebral drowsy squint */
    AutonomicAffectiveContext tired_ctx = {};
    tired_ctx.fatigue = 0.90f;
    setAutonomicAffectiveContext(&tired_ctx);
    for (int i = 0; i < 60; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_tired = getOcularSomaState();
    assert(soma_tired.left_h < 26.0f);
    std::cout << "[PASS] Fatigue-driven palpebral drowsy squint verified: h = " << soma_tired.left_h << std::endl;

    /* Test 4: High arousal surprise widening and oval curvature */
    AutonomicAffectiveContext surprise_ctx = {};
    surprise_ctx.arousal = 0.90f;
    setAutonomicAffectiveContext(&surprise_ctx);
    for (int i = 0; i < 60; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_surprise = getOcularSomaState();
    assert(soma_surprise.left_w > 32.0f);
    assert(soma_surprise.left_n < 3.2f);
    std::cout << "[PASS] High arousal ocular widening & oval rounding verified: w = " << soma_surprise.left_w << ", n = " << soma_surprise.left_n << std::endl;

    /* Test 5: Mischief asymmetrical one-eye sly wink/squint */
    AutonomicAffectiveContext mischief_ctx = {};
    mischief_ctx.mischief = 0.85f;
    setAutonomicAffectiveContext(&mischief_ctx);
    for (int i = 0; i < 60; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_mischief = getOcularSomaState();
    assert(soma_mischief.left_h < soma_mischief.right_h);
    assert(std::fabs(soma_mischief.tilt_left) < 1e-4f && std::fabs(soma_mischief.tilt_right) < 1e-4f);
    std::cout << "[PASS] Mischief asymmetrical sly wink/squint verified: left_h = " << soma_mischief.left_h << ", right_h = " << soma_mischief.right_h << std::endl;

    std::cout << "[PASS] All Autonomic Engine tests passed successfully." << std::endl;
    return 0;
}
