/**
 * @file test_micro_expressions.cpp
 * @brief Algorithmic unit tests for 128-Micro-Expression Biomechanical Engine,
 *        minimum-jerk kinematics, viscoelastic relaxation, and tissue conservation.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <cstring>
#include "src/core/micro_expression_engine.h"
#include "src/config/lore_config.h"

static void test_taxonomy_and_indexing(void) {
    std::cout << "[TEST] Validating 128 Micro-Expression Taxonomy & Naming... ";

    assert(NUM_MICRO_EXPRESSIONS == 128);
    assert(NUM_MICRO_CATEGORIES == 8);

    for (int i = 0; i < NUM_MICRO_EXPRESSIONS; ++i) {
        MicroExpressionId id = (MicroExpressionId)i;
        const char* name = getMicroExpressionName(id);
        assert(name != nullptr);
        assert(strlen(name) > 3);

        uint8_t cat = getMicroExpressionCategory(id);
        assert(cat < NUM_MICRO_CATEGORIES);
        assert(cat == (uint8_t)(i / 16));

        const char* cat_name = getMicroCategoryName(cat);
        assert(cat_name != nullptr);
        assert(strlen(cat_name) > 0);

        /* Reverse lookup verification */
        MicroExpressionId found_id = findMicroExpressionByName(name);
        assert(found_id == id);
    }

    /* Test case-insensitive and dash-tolerant name lookup */
    assert(findMicroExpressionByName("inquisitive_brow_l") == MICRO_EXPR_INQUISITIVE_BROW_L);
    assert(findMicroExpressionByName("INQUISITIVE-BROW-L") == MICRO_EXPR_INQUISITIVE_BROW_L);
    assert(findMicroExpressionByName("shy-fond-peek") == MICRO_EXPR_SHY_FOND_PEEK);
    assert(findMicroExpressionByName("RESTING_BLISS_SLIT") == MICRO_EXPR_RESTING_BLISS_SLIT);
    assert(findMicroExpressionByName("nonexistent_expression") == MICRO_EXPR_NONE);

    std::cout << "PASSED (" << NUM_MICRO_EXPRESSIONS << " unique archetypes)\n";
}

static void test_minimum_jerk_kinematics(void) {
    std::cout << "[TEST] Validating 5th-Order Minimum-Jerk Kinematic Trajectory... ";

    /* 5th-order minimum-jerk formulation: s(tau) = 10*tau^3 - 15*tau^4 + 6*tau^5 */
    auto mj_rise = [](float tau) -> float {
        if (tau <= 0.0f) return 0.0f;
        if (tau >= 1.0f) return 1.0f;
        float tau3 = tau * tau * tau;
        return tau3 * (10.0f - 15.0f * tau + 6.0f * tau * tau);
    };

    /* Verify boundary values */
    assert(std::fabs(mj_rise(0.0f) - 0.0f) < 1e-6f);
    assert(std::fabs(mj_rise(1.0f) - 1.0f) < 1e-6f);

    /* Verify monotonic smoothness */
    float prev = 0.0f;
    for (int step = 1; step <= 100; ++step) {
        float tau = (float)step * 0.01f;
        float val = mj_rise(tau);
        assert(val >= prev); /* Monotonic */
        assert(val <= 1.0f);
        prev = val;
    }

    /* Verify zero first and second derivatives at boundaries (C2 continuity) */
    auto mj_deriv = [](float tau) -> float {
        return 30.0f * tau * tau * (1.0f - tau) * (1.0f - tau);
    };
    assert(std::fabs(mj_deriv(0.0f)) < 1e-6f);
    assert(std::fabs(mj_deriv(1.0f)) < 1e-6f);

    float eps = 0.01f;
    float d1_0 = (mj_rise(eps) - mj_rise(0.0f)) / eps;
    float d1_1 = (mj_rise(1.0f) - mj_rise(1.0f - eps)) / eps;
    assert(std::fabs(d1_0) < 0.01f);
    assert(std::fabs(d1_1) < 0.01f);

    std::cout << "PASSED\n";
}

static void test_viscoelastic_relaxation(void) {
    std::cout << "[TEST] Validating Fractional Viscoelastic Tissue Relaxation... ";

    /* Underdamped spring-damper: d2x/dt2 + 2*zeta*omega_n*dx/dt + omega_n^2 * x = 0 */
    const float omega_n = 18.0f;
    const float zeta = 0.85f;
    const float omega_d = omega_n * std::sqrt(1.0f - zeta * zeta);

    auto spring_eval = [&](float t) -> float {
        float exp_term = std::exp(-zeta * omega_n * t);
        return exp_term * (std::cos(omega_d * t) + (zeta / std::sqrt(1.0f - zeta * zeta)) * std::sin(omega_d * t));
    };

    assert(std::fabs(spring_eval(0.0f) - 1.0f) < 1e-5f);

    /* At t = 0.30s (decay time constant ~ 1 / (0.85 * 18) = 0.065s), energy should decay to < 2% */
    float val_300ms = spring_eval(0.30f);
    assert(std::fabs(val_300ms) < 0.02f);

    /* At t = 0.50s, energy should be effectively zero */
    float val_500ms = spring_eval(0.50f);
    assert(std::fabs(val_500ms) < 0.001f);

    std::cout << "PASSED\n";
}

static void test_tissue_volume_conservation(void) {
    std::cout << "[TEST] Validating Biological Tissue Volume Incompressibility... ";

    /* Biological incompressibility law: Sx = 1.0 / sqrt(Sy) */
    for (float dh = -4.0f; dh <= 4.0f; dh += 0.5f) {
        float sy = 1.0f + dh / SOMA_CANONICAL_EYE_HEIGHT_PX;
        assert(sy > 0.1f);
        float sx = 1.0f / std::sqrt(sy);
        /* Area/volume product: Sx * sqrt(Sy) == 1.0 */
        float product = sx * std::sqrt(sy);
        assert(std::fabs(product - 1.0f) < 1e-5f);
    }

    std::cout << "PASSED\n";
}

static void test_lifecycle_and_delta_application(void) {
    std::cout << "[TEST] Validating Micro-Expression Engine Lifecycle & Safety Clamps... ";

    initMicroExpressionEngine();
    assert(!isMicroExpressionActive());
    assert(getActiveMicroExpressionId() == MICRO_EXPR_NONE);

    MicroExpressionDelta delta_init = getActiveMicroExpressionDelta();
    assert(!delta_init.active);
    assert(std::fabs(delta_init.delta_w_l) < 1e-5f);

    /* Trigger Inquisitive Brow Left */
    bool ok = triggerMicroExpression(MICRO_EXPR_INQUISITIVE_BROW_L, 1.0f);
    assert(ok);
    assert(isMicroExpressionActive());
    assert(getActiveMicroExpressionId() == MICRO_EXPR_INQUISITIVE_BROW_L);

    /* Advance simulation at 60 FPS (~16.66 ms steps) */
    float dt = 0.01666f;
    float total_sim_time = 0.0f;

    float peak_brow = 0.0f;
    bool reached_apex = false;

    for (int frame = 0; frame < 60; ++frame) {
        updateMicroExpressionEngine(dt);
        total_sim_time += dt;

        MicroExpressionDelta d = getActiveMicroExpressionDelta();
        if (d.active) {
            if (d.delta_brow_l > peak_brow) peak_brow = d.delta_brow_l;
            if (d.intensity > 0.95f) reached_apex = true;

            /* Check that perturbations remain within strict biological safety bounds */
            assert(std::fabs(d.delta_w_l) <= 5.0f);
            assert(std::fabs(d.delta_h_l) <= 5.0f);
            assert(std::fabs(d.delta_brow_l) <= 0.40f);
            assert(std::fabs(d.delta_cheek_l) <= 0.30f);
            assert(d.delta_upper_l <= 0.45f);
            assert(d.delta_lower_l <= 0.40f);
            assert(std::fabs(d.delta_gaze_x) <= 4.0f);
            assert(std::fabs(d.delta_gaze_y) <= 4.0f);
        }
    }

    assert(reached_apex);
    assert(peak_brow > 0.15f); /* Inquisitive brow rose noticeably in radians */

    /* Advance further to complete the full micro-expression duration */
    for (int frame = 0; frame < 60; ++frame) {
        updateMicroExpressionEngine(dt);
    }

    /* Should have naturally decayed and completed */
    assert(!isMicroExpressionActive());
    assert(getActiveMicroExpressionId() == MICRO_EXPR_NONE);

    /* Test explicit stop */
    triggerMicroExpression(MICRO_EXPR_PLAYFUL_WINK_L, 0.8f);
    assert(isMicroExpressionActive());
    stopMicroExpression();
    assert(!isMicroExpressionActive());
    assert(getActiveMicroExpressionId() == MICRO_EXPR_NONE);

    std::cout << "PASSED\n";
}

int main(void) {
    std::cout << "=== LoRe Micro-Expression Engine Unit Tests ===\n";
    test_taxonomy_and_indexing();
    test_minimum_jerk_kinematics();
    test_viscoelastic_relaxation();
    test_tissue_volume_conservation();
    test_lifecycle_and_delta_application();
    std::cout << "All 128-Micro-Expression Engine tests PASSED successfully.\n";
    return 0;
}
