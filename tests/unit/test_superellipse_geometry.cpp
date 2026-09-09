/**
 * @file test_superellipse_geometry.cpp
 * @brief Unit test for Superellipse (Formula Lamé) parametric rendering math and bounds safety.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

struct SuperellipseParams {
    float xc = 40.0f;
    float yc = 32.0f;
    float a = 15.0f; /* semi-width */
    float b = 14.0f; /* semi-height */
    float n = 4.2f;  /* Lamé exponent */
    float theta = 0.05f; /* Tilt angle */
    float stroke = 0.0f; /* 0 = solid */
};

/* Fast rasterizer test on a 128x64 virtual monochrome bitmap */
void rasterizeSuperellipse(const SuperellipseParams& p, std::vector<uint8_t>& buf) {
    float cos_t = std::cos(p.theta);
    float sin_t = std::sin(p.theta);
    float a = std::max(1.0f, p.a);
    float b = std::max(1.0f, p.b);
    float n = std::max(1.0f, p.n);

    float a_in = a - p.stroke;
    float b_in = b - p.stroke;
    bool is_hollow = (p.stroke > 0.5f && a_in > 1.0f && b_in > 1.0f);

    /* Compute conservative bounding box */
    float r_max = std::sqrt(a * a + b * b) + 1.0f;
    int x_min = std::max(0, (int)std::floor(p.xc - r_max));
    int x_max = std::min(127, (int)std::ceil(p.xc + r_max));
    int y_min = std::max(0, (int)std::floor(p.yc - r_max));
    int y_max = std::min(63, (int)std::ceil(p.yc + r_max));

    for (int y = y_min; y <= y_max; ++y) {
        float dy = ((float)y + 0.5f) - p.yc;
        for (int x = x_min; x <= x_max; ++x) {
            float dx = ((float)x + 0.5f) - p.xc;
            float xr = dx * cos_t + dy * sin_t;
            float yr = -dx * sin_t + dy * cos_t;

            float u = std::fabs(xr) / a;
            float v = std::fabs(yr) / b;
            if (u > 1.0f || v > 1.0f) continue;

            float d = std::pow(u, n) + std::pow(v, n);
            if (d <= 1.0f) {
                if (is_hollow) {
                    float u_in = std::fabs(xr) / a_in;
                    float v_in = std::fabs(yr) / b_in;
                    if (u_in <= 1.0f && v_in <= 1.0f) {
                        float d_in = std::pow(u_in, n) + std::pow(v_in, n);
                        if (d_in < 1.0f) {
                            continue; /* Inside inner hole */
                        }
                    }
                }
                buf[y * 128 + x] = 1;
            }
        }
    }
}

int main() {
    std::cout << "[TEST] Running Superellipse geometry and rasterization unit tests..." << std::endl;

    std::vector<uint8_t> buffer(128 * 64, 0);

    /* Test 1: Solid squircle (LoRe classic) */
    SuperellipseParams p1;
    p1.xc = 40.0f;
    p1.yc = 32.0f;
    p1.a = 16.0f;
    p1.b = 15.0f;
    p1.n = 4.2f;
    p1.theta = 0.0f;
    p1.stroke = 0.0f;

    rasterizeSuperellipse(p1, buffer);

    /* Center pixel must be 1 */
    assert(buffer[32 * 128 + 40] == 1);
    /* Far corner pixel must be 0 */
    assert(buffer[0 * 128 + 0] == 0);

    /* Count filled pixels */
    int solid_count = 0;
    for (uint8_t px : buffer) solid_count += px;
    assert(solid_count > 600 && solid_count < 1000);
    std::cout << "[PASS] Solid superellipse rasterized (" << solid_count << " pixels)." << std::endl;

    /* Test 2: Hollow stroke */
    std::fill(buffer.begin(), buffer.end(), 0);
    SuperellipseParams p2 = p1;
    p2.stroke = 2.0f;
    rasterizeSuperellipse(p2, buffer);

    /* Center pixel must be 0 (hollow hole) */
    assert(buffer[32 * 128 + 40] == 0);
    /* Border pixel must be 1 */
    assert(buffer[32 * 128 + 40 + 15] == 1);

    int hollow_count = 0;
    for (uint8_t px : buffer) hollow_count += px;
    assert(hollow_count > 100 && hollow_count < solid_count);
    std::cout << "[PASS] Hollow superellipse outline rasterized (" << hollow_count << " pixels)." << std::endl;

    /* Test 3: Rotated tilted ellipse */
    std::fill(buffer.begin(), buffer.end(), 0);
    SuperellipseParams p3 = p1;
    p3.theta = 0.25f; /* ~14 degrees */
    rasterizeSuperellipse(p3, buffer);
    assert(buffer[32 * 128 + 40] == 1);
    std::cout << "[PASS] Rotated tilted superellipse verified." << std::endl;

    std::cout << "[PASS] All Superellipse geometry tests passed successfully." << std::endl;
    return 0;
}
