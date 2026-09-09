/**
 * @file test_autonomic_cpg.cpp
 * @brief Host unit test suite for Autonomic Engine, Matsuoka CPG, and Ocular Soma dynamics.
 */

#include "src/ai/autonomic_engine.h"
#include <iostream>
#include <cassert>
#include <cmath>

/* Standalone test definitions for global ocular gaze offsets and expression */
float g_currentOffsetX = 0.0f;
float g_currentOffsetY = 0.0f;
Expression g_currentExpr = EXPR_IDLE;

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

    /* Verify Listing's Law Axial Torsion on diagonal eccentric gaze */
    g_currentOffsetX = 14.0f;
    g_currentOffsetY = 8.0f;
    updateAutonomicEngine(dt);
    OcularSomaState soma_diag = getOcularSomaState();
    assert(soma_diag.tilt_left > 0.015f && soma_diag.tilt_left < 0.045f);
    assert(std::fabs(soma_diag.tilt_left - soma_diag.tilt_right) < 1e-6f);

    /* Negative diagonal quadrant produces inverted torsion */
    g_currentOffsetX = -14.0f;
    g_currentOffsetY = 8.0f;
    updateAutonomicEngine(dt);
    OcularSomaState soma_diag_inv = getOcularSomaState();
    assert(soma_diag_inv.tilt_left < -0.015f && soma_diag_inv.tilt_left > -0.045f);

    /* Cardinal primary gaze (0, 0) yields zero torsional tilt */
    g_currentOffsetX = 0.0f;
    g_currentOffsetY = 0.0f;
    updateAutonomicEngine(dt);
    OcularSomaState soma_primary = getOcularSomaState();
    assert(std::fabs(soma_primary.tilt_left) < 1e-6f);
    assert(std::fabs(soma_primary.tilt_right) < 1e-6f);
    std::cout << "[PASS] Listing's Law axial torsion kinematics verified across primary and tertiary quadrants." << std::endl;

    /* Verify continuous morphing into EXPR_ANGRY */
    g_currentExpr = EXPR_ANGRY;
    for (int i = 0; i < 30; ++i) updateAutonomicEngine(dt); /* ~500 ms, full convergence */
    OcularSomaState soma_angry = getOcularSomaState();
    assert(soma_angry.brow_tilt_left > 0.30f);
    assert(soma_angry.brow_tilt_right < -0.30f);
    assert(soma_angry.upper_lid_left > 0.20f);
    std::cout << "[PASS] Continuous morphing to EXPR_ANGRY inward-slanted brow plane verified." << std::endl;

    /* Verify continuous morphing into EXPR_SAD */
    g_currentExpr = EXPR_SAD;
    for (int i = 0; i < 30; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_sad = getOcularSomaState();
    assert(soma_sad.brow_tilt_left < -0.20f);
    assert(soma_sad.brow_tilt_right > 0.20f);
    std::cout << "[PASS] Continuous morphing to EXPR_SAD dejected outer-drooping brow plane verified." << std::endl;

    /* Verify continuous morphing into EXPR_HAPPY */
    g_currentExpr = EXPR_HAPPY;
    for (int i = 0; i < 30; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_happy = getOcularSomaState();
    assert(soma_happy.lower_lid_left > 0.35f);
    assert(soma_happy.lower_lid_right > 0.35f);
    std::cout << "[PASS] Continuous morphing to EXPR_HAPPY raised lower cheek plane verified." << std::endl;

    /* Verify continuous morphing into EXPR_SLEEPY */
    g_currentExpr = EXPR_SLEEPY;
    for (int i = 0; i < 30; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_sleepy = getOcularSomaState();
    assert(soma_sleepy.upper_lid_left > 0.35f);
    assert(soma_sleepy.upper_lid_right > 0.35f);
    std::cout << "[PASS] Continuous morphing to EXPR_SLEEPY heavy upper lid droop verified." << std::endl;

    /* Verify continuous morphing into EXPR_DIZZY (Vestibular Disturbance & Tissue Conservation) */
    g_currentExpr = EXPR_DIZZY;
    for (int i = 0; i < 30; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_dizzy = getOcularSomaState();
    assert(soma_dizzy.left_w > 24.0f && soma_dizzy.left_w < 34.0f);
    assert(soma_dizzy.left_h > 22.0f && soma_dizzy.left_h < 34.0f);
    assert(soma_dizzy.left_n > 1.8f && soma_dizzy.left_n < 2.3f);
    /* Verify biological tissue incompressibility conservation: (Sx * sqrt(Sy) ~ 1.0) */
    float s_x = soma_dizzy.left_w / 28.0f;
    float s_y = soma_dizzy.left_h / 28.0f;
    float incompressibility_residual = std::fabs(s_x * std::sqrt(s_y) - 1.0f);
    assert(incompressibility_residual < 0.12f);
    std::cout << "[PASS] Continuous morphing to EXPR_DIZZY vestibular somatic law verified." << std::endl;

    /* Return to baseline EXPR_IDLE */
    g_currentExpr = EXPR_IDLE;
    for (int i = 0; i < 30; ++i) updateAutonomicEngine(dt);
    OcularSomaState soma_idle = getOcularSomaState();
    assert(std::fabs(soma_idle.brow_tilt_left) < 0.05f);
    assert(std::fabs(soma_idle.brow_tilt_right) < 0.05f);
    assert(soma_idle.upper_lid_left < 0.05f);
    assert(soma_idle.lower_lid_left < 0.05f);
    std::cout << "[PASS] Continuous recovery to EXPR_IDLE baseline resting state verified." << std::endl;

    std::cout << "[PASS] All Autonomic Engine tests passed successfully." << std::endl;
    return 0;
}
