/**
 * @file facial_renderer.cpp
 * @brief 1-bit OLED facial expression rendering and natural transition easing for LoRe.
 */

#include "src/core/facial_renderer.h"
#include "src/core/facial_bitmaps.h"
#include "src/core/display_engine.h"
#include "src/core/gaze_engine.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include "src/math/affective_engine.h"
#include "src/math/kinematics.h"
#include "src/ai/autonomic_engine.h"
#include "src/ai/brain_engine.h"
#include <Arduino.h>
#include <math.h>

static LGFX_Sprite* s_canvas_ptr = &canvas;

/* Ambient Micro-Particle Accent System */
enum ParticleType {
    PARTICLE_NONE = 0,
    PARTICLE_ZZZ,
    PARTICLE_HEART,
    PARTICLE_SWEAT,
    PARTICLE_STAR
};

struct OcularParticle {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float decay_rate;
    ParticleType type;
    bool active;
};

static const int kMaxParticles = 4;
static OcularParticle s_particles[kMaxParticles] = {};
static uint32_t s_lastParticleSpawnTime = 0;
static uint32_t s_lastParticleUpdateTimeUs = 0;

static const uint32_t kZzzSpawnIntervalMs = 1300;
static const uint32_t kHeartSpawnIntervalMs = 2200;
static const uint32_t kSweatSpawnIntervalMs = 1900;
static const uint32_t kStarSpawnIntervalMs = 1100;

static void spawnParticle(ParticleType type, float x, float y, float vx, float vy, float decay_rate) {
    for (int i = 0; i < kMaxParticles; ++i) {
        if (!s_particles[i].active) {
            s_particles[i].x = x;
            s_particles[i].y = y;
            s_particles[i].vx = vx;
            s_particles[i].vy = vy;
            s_particles[i].life = 1.0f;
            s_particles[i].decay_rate = decay_rate;
            s_particles[i].type = type;
            s_particles[i].active = true;
            return;
        }
    }
}

void clearOcularParticles(void) {
    for (int i = 0; i < kMaxParticles; ++i) {
        s_particles[i].active = false;
    }
    s_lastParticleSpawnTime = 0;
}

void set_facial_canvas(LGFX_Sprite* p_canvas) {
    if (p_canvas) {
        s_canvas_ptr = p_canvas;
    }
}

static void renderOneEyeSuperellipse(LGFX_Sprite& cv, float xc, float yc, float a, float b, float n, float theta, float stroke,
                                    float brow_tilt, float cheek_tilt, float upper_lid, float lower_lid) {
    if (b <= 1.5f || (1.0f - upper_lid - lower_lid) <= 0.05f) {
        int slit_w = (int)roundf(a * 2.0f);
        if (slit_w < 10) slit_w = 10;
        int left_pos = (int)roundf(xc - a);
        cv.fillRoundRect(left_pos, (int)roundf(yc - 1.0f), slit_w, 2, 1, TFT_WHITE);
        return;
    }

    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    a = fmaxf(1.0f, a);
    b = fmaxf(1.0f, b);
    n = fmaxf(1.0f, n);

    float a_in = a - stroke;
    float b_in = b - stroke;
    bool is_hollow = (stroke > 0.5f && a_in > 1.0f && b_in > 1.0f);

    float y_top_cut = b * (1.0f - upper_lid);
    float y_bottom_cut = b * (1.0f - lower_lid);
    float tan_brow = tanf(brow_tilt);
    float tan_cheek = tanf(cheek_tilt);

    float r_max = sqrtf(a * a + b * b) + 1.0f;
    int x_min = (int)fmaxf(0.0f, floorf(xc - r_max));
    int x_max = (int)fminf((float)(OLED_PANEL_WIDTH_PX - 1), ceilf(xc + r_max));
    int y_min = (int)fmaxf(0.0f, floorf(yc - r_max));
    int y_max = (int)fminf((float)(OLED_PANEL_HEIGHT_PX - 1), ceilf(yc + r_max));

    for (int y = y_min; y <= y_max; ++y) {
        float dy = ((float)y + 0.5f) - yc;
        for (int x = x_min; x <= x_max; ++x) {
            float dx = ((float)x + 0.5f) - xc;
            float xr = dx * cos_t + dy * sin_t;
            float yr = -dx * sin_t + dy * cos_t;

            /* Fast early rejection against angled brow and cheek palpebral cutting planes */
            if (yr < -y_top_cut + xr * tan_brow) continue;
            if (yr > y_bottom_cut + xr * tan_cheek) continue;

            float u = fabsf(xr) / a;
            float v = fabsf(yr) / b;
            if (u > 1.0f || v > 1.0f) continue;

            float d = powf(u, n) + powf(v, n);
            if (d <= 1.0f) {
                if (is_hollow) {
                    float u_in = fabsf(xr) / a_in;
                    float v_in = fabsf(yr) / b_in;
                    if (u_in <= 1.0f && v_in <= 1.0f) {
                        float d_in = powf(u_in, n) + powf(v_in, n);
                        if (d_in < 1.0f) continue;
                    }
                }
                cv.drawPixel(x, y, TFT_WHITE);
            }
        }
    }
}

static const float kDizzyWobbleAmplitude = 2.5f;
static const float kDizzyWobbleSpeed = 0.006f;
static const float kDizzySpiralSpeed = 0.007f;
static const float kSpiralMinRadius = 1.8f;
static const float kSpiralRadialMargin = 2.5f;
static const int kSpiralSteps = 28;

static void renderDizzySpiralEye(LGFX_Sprite& cv, float xc, float yc, float max_r, float aperture, float phase_rad) {
    if (aperture <= 0.05f || max_r < 3.0f) {
        int slit_w = (int)roundf(max_r * 2.0f);
        if (slit_w < 10) slit_w = 10;
        cv.fillRoundRect((int)roundf(xc - max_r), (int)roundf(yc - 1.0f), slit_w, 2, 1, TFT_WHITE);
        return;
    }

    int rx = (int)roundf(max_r);
    int ry = (int)roundf(max_r * aperture);
    if (rx < 4) rx = 4;
    if (ry < 2) ry = 2;

    /* Outer elliptical perimeter */
    cv.drawEllipse((int)roundf(xc), (int)roundf(yc), rx, ry, TFT_WHITE);

    /* Solid pupil core at center */
    int icx = (int)roundf(xc);
    int icy = (int)roundf(yc);
    cv.fillRect(icx - 1, icy - 1, 2, 2, TFT_WHITE);

    /* Archimedean spiral: r(theta) = r_min + (r_max - r_min) * (theta / theta_max) */
    const float kMaxTheta = 4.0f * 3.14159265f;
    const float r_max = fmaxf(kSpiralMinRadius + 1.0f, max_r - kSpiralRadialMargin);
    const float d_theta = kMaxTheta / (float)kSpiralSteps;

    float prev_x = xc + kSpiralMinRadius * cosf(phase_rad);
    float prev_y = yc + kSpiralMinRadius * sinf(phase_rad) * aperture;

    for (int i = 1; i <= kSpiralSteps; ++i) {
        float theta = (float)i * d_theta;
        float r = kSpiralMinRadius + (r_max - kSpiralMinRadius) * (theta / kMaxTheta);
        float angle = theta + phase_rad;
        float curr_x = xc + r * cosf(angle);
        float curr_y = yc + r * sinf(angle) * aperture;

        int x0 = (int)roundf(prev_x);
        int y0 = (int)roundf(prev_y);
        int x1 = (int)roundf(curr_x);
        int y1 = (int)roundf(curr_y);

        cv.drawLine(x0, y0, x1, y1, TFT_WHITE);
        cv.drawLine(x0, y0 + 1, x1, y1 + 1, TFT_WHITE);

        prev_x = curr_x;
        prev_y = curr_y;
    }
}

static void updateAndRenderParticles(LGFX_Sprite& cv, float left_xc, float left_yc, float right_xc, float right_yc,
                                    float left_a, float left_b, float right_a, float right_b, float aperture) {
    (void)left_b;
    (void)right_b;
    (void)aperture;
    uint32_t now = millis();
    uint32_t nowUs = micros();
    float dt = (s_lastParticleUpdateTimeUs > 0) ? (float)(nowUs - s_lastParticleUpdateTimeUs) * 0.000001f : 0.01666f;
    if (dt < 0.005f) dt = 0.005f;
    if (dt > 0.050f) dt = 0.050f;
    s_lastParticleUpdateTimeUs = nowUs;

    /* Autonomous Particle Spawning */
    if (g_currentExpr == EXPR_SLEEPY || isDrowsyStruggleActive()) {
        if (now - s_lastParticleSpawnTime >= kZzzSpawnIntervalMs) {
            s_lastParticleSpawnTime = now;
            float spawn_x = right_xc + right_a * 0.6f + (float)(esp_random() % 6);
            float spawn_y = right_yc - 10.0f - (float)(esp_random() % 4);
            float vx = 3.5f + (float)(esp_random() % 15) * 0.1f;
            float vy = -5.0f - (float)(esp_random() % 15) * 0.1f;
            spawnParticle(PARTICLE_ZZZ, spawn_x, spawn_y, vx, vy, 0.45f);
        }
    } else if (g_currentExpr == EXPR_HAPPY) {
        float bonding = getBrainBondingLevel();
        if (bonding >= 0.55f && (now - s_lastParticleSpawnTime >= kHeartSpawnIntervalMs)) {
            s_lastParticleSpawnTime = now;
            float spawn_x = right_xc + right_a + 2.0f + (float)(esp_random() % 4);
            float spawn_y = right_yc - (float)(esp_random() % 6);
            float vx = 1.0f + (float)(esp_random() % 10) * 0.1f;
            float vy = -4.5f - (float)(esp_random() % 10) * 0.1f;
            spawnParticle(PARTICLE_HEART, spawn_x, spawn_y, vx, vy, 0.50f);
        }
    } else if (g_currentExpr == EXPR_SUSPICIOUS || g_currentExpr == EXPR_SURPRISED) {
        if (now - s_lastParticleSpawnTime >= kSweatSpawnIntervalMs) {
            s_lastParticleSpawnTime = now;
            float spawn_x = left_xc - left_a - 4.0f;
            float spawn_y = left_yc - 8.0f;
            float vx = -0.5f;
            float vy = 4.0f + (float)(esp_random() % 10) * 0.1f;
            spawnParticle(PARTICLE_SWEAT, spawn_x, spawn_y, vx, vy, 0.70f);
        }
    } else if (g_currentExpr == EXPR_DIZZY) {
        if (now - s_lastParticleSpawnTime >= kStarSpawnIntervalMs) {
            s_lastParticleSpawnTime = now;
            static bool s_star_side = false;
            s_star_side = !s_star_side;
            float base_xc = s_star_side ? left_xc : right_xc;
            float offset_x = s_star_side ? (-left_a - 3.0f) : (right_a + 3.0f);
            float spawn_x = base_xc + offset_x + (float)(esp_random() % 5) - 2.0f;
            float spawn_y = (s_star_side ? left_yc : right_yc) - 8.0f - (float)(esp_random() % 4);
            float vx = (s_star_side ? -1.5f : 1.5f) + (float)(esp_random() % 10) * 0.1f;
            float vy = -3.5f - (float)(esp_random() % 10) * 0.1f;
            spawnParticle(PARTICLE_STAR, spawn_x, spawn_y, vx, vy, 0.55f);
        }
    }

    /* Update & Render Active Particles */
    for (int i = 0; i < kMaxParticles; ++i) {
        if (!s_particles[i].active) continue;

        OcularParticle& p = s_particles[i];
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= p.decay_rate * dt;

        if (p.life <= 0.0f || p.y < -6.0f || p.y > (float)(OLED_PANEL_HEIGHT_PX + 6) ||
            p.x < -6.0f || p.x > (float)(OLED_PANEL_WIDTH_PX + 6)) {
            p.active = false;
            continue;
        }

        int px = (int)roundf(p.x);
        int py = (int)roundf(p.y);

        switch (p.type) {
            case PARTICLE_ZZZ: {
                if (px >= 0 && px <= OLED_PANEL_WIDTH_PX - 4 && py >= 0 && py <= OLED_PANEL_HEIGHT_PX - 4) {
                    cv.drawFastHLine(px, py, 4, TFT_WHITE);
                    cv.drawLine(px + 3, py, px, py + 3, TFT_WHITE);
                    cv.drawFastHLine(px, py + 3, 4, TFT_WHITE);
                }
                break;
            }
            case PARTICLE_HEART: {
                int hx = px + (int)roundf(sinf(p.life * 6.28318f) * 1.5f);
                int hy = py;
                if (hx >= 0 && hx <= OLED_PANEL_WIDTH_PX - 5 && hy >= 0 && hy <= OLED_PANEL_HEIGHT_PX - 5) {
                    cv.drawPixel(hx + 1, hy, TFT_WHITE);
                    cv.drawPixel(hx + 3, hy, TFT_WHITE);
                    cv.drawFastHLine(hx, hy + 1, 5, TFT_WHITE);
                    cv.drawFastHLine(hx, hy + 2, 5, TFT_WHITE);
                    cv.drawFastHLine(hx + 1, hy + 3, 3, TFT_WHITE);
                    cv.drawPixel(hx + 2, hy + 4, TFT_WHITE);
                }
                break;
            }
            case PARTICLE_SWEAT: {
                if (px >= 0 && px <= OLED_PANEL_WIDTH_PX - 3 && py >= 0 && py <= OLED_PANEL_HEIGHT_PX - 4) {
                    cv.drawPixel(px + 1, py, TFT_WHITE);
                    cv.drawFastHLine(px, py + 1, 3, TFT_WHITE);
                    cv.drawFastHLine(px, py + 2, 3, TFT_WHITE);
                    cv.drawPixel(px + 1, py + 3, TFT_WHITE);
                }
                break;
            }
            case PARTICLE_STAR: {
                int sx = px + (int)roundf(cosf(p.life * 12.56637f) * 1.5f);
                int sy = py;
                if (sx >= 2 && sx <= OLED_PANEL_WIDTH_PX - 3 && sy >= 2 && sy <= OLED_PANEL_HEIGHT_PX - 3) {
                    cv.drawFastVLine(sx, sy - 2, 5, TFT_WHITE);
                    cv.drawFastHLine(sx - 2, sy, 5, TFT_WHITE);
                    if (fmodf(p.life * 10.0f, 2.0f) < 1.0f) {
                        cv.drawPixel(sx - 1, sy - 1, TFT_WHITE);
                        cv.drawPixel(sx + 1, sy + 1, TFT_WHITE);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void drawAutonomousSoma(const OcularSomaState& soma, float offsetX, float offsetY, float palpebralAperture, float vergence) {
    if (!s_canvas_ptr) return;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int ox = getFilteredOx(offsetX) + get_burn_shift_x();
    int oy = getFilteredOy(offsetY) + get_burn_shift_y();
    int v_px = (int)roundf(vergence);

    cv.fillScreen(TFT_BLACK);

    float aperture = constrain(palpebralAperture, 0.0f, 1.0f);

    float left_xc = soma.left_x + (float)ox + (float)v_px + soma.nystagmus_x;
    float left_yc = soma.left_y + (float)oy + soma.nystagmus_y;
    float right_xc = soma.right_x + (float)ox - (float)v_px + soma.nystagmus_x;
    float right_yc = soma.right_y + (float)oy + soma.nystagmus_y;

    float left_a = soma.left_w * 0.5f;
    float left_b = soma.left_h * 0.5f * aperture;
    float right_a = soma.right_w * 0.5f;
    float right_b = soma.right_h * 0.5f * aperture;

    if (g_currentExpr == EXPR_DIZZY) {
        float t_ms = (float)millis();
        float wobble_rad = t_ms * kDizzyWobbleSpeed;
        float phase_l = t_ms * kDizzySpiralSpeed;
        float phase_r = -t_ms * kDizzySpiralSpeed;

        /* Asynchronous out-of-phase orbital wobble to simulate rolling dizzy motion */
        float wobble_x_l = cosf(wobble_rad) * kDizzyWobbleAmplitude;
        float wobble_y_l = sinf(wobble_rad) * kDizzyWobbleAmplitude;
        float wobble_x_r = cosf(wobble_rad + 3.14159265f) * kDizzyWobbleAmplitude;
        float wobble_y_r = sinf(wobble_rad + 3.14159265f) * kDizzyWobbleAmplitude;

        renderDizzySpiralEye(cv, left_xc + wobble_x_l, left_yc + wobble_y_l, left_a, aperture, phase_l);
        renderDizzySpiralEye(cv, right_xc + wobble_x_r, right_yc + wobble_y_r, right_a, aperture, phase_r);
    } else {
        renderOneEyeSuperellipse(cv, left_xc, left_yc, left_a, left_b, soma.left_n, soma.tilt_left, soma.stroke_thickness,
                                soma.brow_tilt_left, soma.cheek_tilt_left, soma.upper_lid_left, soma.lower_lid_left);
        renderOneEyeSuperellipse(cv, right_xc, right_yc, right_a, right_b, soma.right_n, soma.tilt_right, soma.stroke_thickness,
                                soma.brow_tilt_right, soma.cheek_tilt_right, soma.upper_lid_right, soma.lower_lid_right);
    }

    /* Render subtle expressive accents */
    if (g_currentExpr == EXPR_CRYING && aperture > 0.3f) {
        static uint8_t s_tear_frame = 0;
        s_tear_frame = (s_tear_frame + 1) % 40;
        int tear_y_l = (int)roundf(left_yc + left_b) + (s_tear_frame % 14);
        int tear_y_r = (int)roundf(right_yc + right_b) + ((s_tear_frame + 7) % 14);
        int tear_x_l = (int)roundf(left_xc - left_a * 0.35f);
        int tear_x_r = (int)roundf(right_xc + right_a * 0.35f);
        if (tear_y_l < OLED_PANEL_HEIGHT_PX - 2) cv.fillRoundRect(tear_x_l - 1, tear_y_l, 2, 4, 1, TFT_WHITE);
        if (tear_y_r < OLED_PANEL_HEIGHT_PX - 2) cv.fillRoundRect(tear_x_r - 1, tear_y_r, 2, 4, 1, TFT_WHITE);
    } else if ((g_currentExpr == EXPR_HAPPY || g_currentExpr == EXPR_COOL) && aperture > 0.4f) {
        int b_l = (int)roundf(left_xc - left_a - 4.0f);
        int b_r = (int)roundf(right_xc + right_a + 2.0f);
        int b_y = (int)roundf(left_yc + left_b * 0.35f);
        if (b_l >= 2 && b_y >= 0 && b_y < OLED_PANEL_HEIGHT_PX) {
            cv.drawPixel(b_l, b_y, TFT_WHITE);
            cv.drawPixel(b_l + 2, b_y, TFT_WHITE);
        }
        if (b_r < OLED_PANEL_WIDTH_PX - 3 && b_y >= 0 && b_y < OLED_PANEL_HEIGHT_PX) {
            cv.drawPixel(b_r, b_y, TFT_WHITE);
            cv.drawPixel(b_r + 2, b_y, TFT_WHITE);
        }
    } else if (g_currentExpr == EXPR_ANGRY && aperture > 0.4f) {
        int v_x = (int)roundf(right_xc + right_a + 5.0f);
        int v_y = (int)roundf(right_yc - right_b * 0.5f);
        if (v_x < OLED_PANEL_WIDTH_PX - 4 && v_y > 4 && v_y < OLED_PANEL_HEIGHT_PX - 4) {
            cv.drawLine(v_x - 2, v_y, v_x + 2, v_y, TFT_WHITE);
            cv.drawLine(v_x, v_y - 2, v_x, v_y + 2, TFT_WHITE);
        }
    }

    /* Render ambient micro-particles */
    updateAndRenderParticles(cv, left_xc, left_yc, right_xc, right_yc,
                            left_a, left_b, right_a, right_b, aperture);

    cv.pushSprite(0, 0);
}

void drawFace(Expression expr, float eyeHeightFactor, float offsetX, float offsetY, float frame, float vergence, float scale) {
    (void)frame;
    if (!s_canvas_ptr) return;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int ox = getFilteredOx(offsetX) + get_burn_shift_x();
    int oy = getFilteredOy(offsetY) + get_burn_shift_y();

    cv.fillScreen(TFT_BLACK);

    /* Determine effective horizontal and vertical scaling factors */
    float scaleY = g_currentEyeScaleY;
    float scaleX = g_currentEyeScaleX;
    if (fabsf(scale - 1.0f) > 0.001f && fabsf(scale - g_currentEyeScaleY) > 0.001f) {
        scaleY = scale;
        scaleX = (scaleY > 0.1f) ? (1.0f / sqrtf(scaleY)) : 1.0f;
    }

    /* Compute affective Duchenne smile softening from current positive valence */
    float valence = getEmotionValence();
    float d_duchenne = 0.0f;
    if (valence > 0.20f) {
        d_duchenne = 3.0f * constrain((valence - 0.20f) / 0.70f, 0.0f, 1.0f);
    }

    float h_clamped = constrain(eyeHeightFactor, 0.0f, 1.0f);
    float c = 1.0f - h_clamped; /* Palpebral closure progress: 0.0 = fully open, 1.0 = fully closed */

    /* Calculate dynamic eye dimensions and stereoscopic vergence centers */
    int v_px = (int)roundf(vergence);
    int eye_w = (int)roundf(SOMA_CANONICAL_EYE_WIDTH_PX * scaleX);
    eye_w = constrain(eye_w, 24, 40);

    /* Left eye center: SOMA_CANONICAL_LEFT_X + ox + v; Right eye center: SOMA_CANONICAL_RIGHT_X + ox - v */
    int left_x = ((int)roundf(SOMA_CANONICAL_LEFT_X) + ox + v_px) - (eye_w / 2);
    int right_x = ((int)roundf(SOMA_CANONICAL_RIGHT_X) + ox - v_px) - (eye_w / 2);

    if (h_clamped <= 0.05f) {
        /* Render closed eyelid slits during blinks and peaceful deep sleep */
        cv.fillRoundRect(left_x, 31 + oy, eye_w, 2, 1, TFT_WHITE);
        cv.fillRoundRect(right_x, 31 + oy, eye_w, 2, 1, TFT_WHITE);
    } else {
        bool isBaselineResting = (h_clamped >= 0.99f) &&
                                 (v_px == 0) &&
                                 (fabsf(scaleX - 1.0f) < 0.02f) &&
                                 (fabsf(scaleY - 1.0f) < 0.02f) &&
                                 (d_duchenne < 0.20f);

        switch (expr) {
            case EXPR_HAPPY:
                if (isBaselineResting) {
                    /* Bit-exact reproduction of Lopaka monochrome bitmap when resting at baseline */
                    cv.drawXBitmap(FACE_HAPPY_BASE_X + ox, FACE_HAPPY_BASE_Y + oy, FACE_HAPPY_BITS, FACE_HAPPY_WIDTH, FACE_HAPPY_HEIGHT, TFT_WHITE);
                } else {
                    int y_top = (26 + oy) + (int)roundf(5.0f * powf(c, 0.75f));
                    int y_bottom = (37 + oy) - (int)roundf(5.0f * powf(c, 1.50f));
                    int curH = y_bottom - y_top + 1;
                    if (curH <= 3) {
                        cv.fillRoundRect(left_x, 31 + oy, eye_w, 2, 1, TFT_WHITE);
                        cv.fillRoundRect(right_x, 31 + oy, eye_w, 2, 1, TFT_WHITE);
                    } else if (h_clamped >= 0.99f && v_px == 0) {
                        cv.drawXBitmap(FACE_HAPPY_BASE_X + ox, FACE_HAPPY_BASE_Y + oy, FACE_HAPPY_BITS, FACE_HAPPY_WIDTH, FACE_HAPPY_HEIGHT, TFT_WHITE);
                    } else if (h_clamped >= 0.99f) {
                        cv.drawXBitmap(left_x, 26 + oy, FACE_HAPPY_EYE_LEFT_BITS, 32, 12, TFT_WHITE);
                        cv.drawXBitmap(right_x, 26 + oy, FACE_HAPPY_EYE_RIGHT_BITS, 32, 12, TFT_WHITE);
                    } else {
                        cv.setClipRect(0, y_top, OLED_PANEL_WIDTH_PX, curH);
                        cv.drawXBitmap(left_x, 26 + oy, FACE_HAPPY_EYE_LEFT_BITS, 32, 12, TFT_WHITE);
                        cv.drawXBitmap(right_x, 26 + oy, FACE_HAPPY_EYE_RIGHT_BITS, 32, 12, TFT_WHITE);
                        cv.clearClipRect();
                    }
                }
                updateAndRenderParticles(cv, (float)left_x + eye_w * 0.5f, 31.0f + (float)oy,
                                        (float)right_x + eye_w * 0.5f, 31.0f + (float)oy,
                                        eye_w * 0.5f, 8.0f, eye_w * 0.5f, 8.0f, h_clamped);
                break;

            case EXPR_IDLE:
            default:
                drawAutonomousSoma(getOcularSomaState(), offsetX, offsetY, h_clamped, vergence);
                return;
        }
    }

    cv.pushSprite(0, 0);
}

void drawMiniFace(Expression expr, float eyeHeightFactor, float offsetX, float offsetY, float scale) {
    (void)scale;
    if (!s_canvas_ptr) return;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int ox = (int)roundf(offsetX);
    int oy = (int)roundf(offsetY);

    /* Centered dual mini-eyes in top status band y = 0..24 */
    int lx = 44 + ox;
    int rx = 84 + ox;
    int ly = 12 + oy;
    int ry = 12 + oy;

    if (eyeHeightFactor <= 0.05f) {
        cv.fillRoundRect(lx - 7, ly, 14, 2, 1, TFT_WHITE);
        cv.fillRoundRect(rx - 7, ry, 14, 2, 1, TFT_WHITE);
    } else if (expr == EXPR_HAPPY) {
        if (eyeHeightFactor <= 0.25f) {
            cv.fillRoundRect(lx - 7, ly, 14, 2, 1, TFT_WHITE);
            cv.fillRoundRect(rx - 7, ry, 14, 2, 1, TFT_WHITE);
        } else {
            /* Compact cheerful mini-eyes (caret shape) */
            cv.drawLine(lx - 5, ly + 2, lx, ly - 2, TFT_WHITE);
            cv.drawLine(lx, ly - 2, lx + 5, ly + 2, TFT_WHITE);
            cv.drawLine(rx - 5, ry + 2, rx, ry - 2, TFT_WHITE);
            cv.drawLine(rx, ry - 2, rx + 5, ry + 2, TFT_WHITE);
        }
    } else if (expr == EXPR_DIZZY) {
        /* Compact dizzy swirl mini-eyes for top status band */
        float t_ms = (float)millis();
        float phase_l = t_ms * kDizzySpiralSpeed;
        float phase_r = -t_ms * kDizzySpiralSpeed;
        cv.drawCircle(lx, ly, 5, TFT_WHITE);
        cv.drawCircle(rx, ry, 5, TFT_WHITE);
        int dx_l = (int)roundf(cosf(phase_l) * 3.5f);
        int dy_l = (int)roundf(sinf(phase_l) * 3.5f);
        cv.drawLine(lx - dx_l, ly - dy_l, lx + dx_l, ly + dy_l, TFT_WHITE);
        int dx_r = (int)roundf(cosf(phase_r) * 3.5f);
        int dy_r = (int)roundf(sinf(phase_r) * 3.5f);
        cv.drawLine(rx - dx_r, ry - dy_r, rx + dx_r, ry + dy_r, TFT_WHITE);
    } else {
        if (eyeHeightFactor >= 0.99f) {
            /* Compact resting mini-eyes (rounded rectangles) */
            cv.fillRoundRect(lx - 7, ly - 5, 14, 10, 3, TFT_WHITE);
            cv.fillRoundRect(rx - 7, ry - 5, 14, 10, 3, TFT_WHITE);
        } else {
            float h_clamped = constrain(eyeHeightFactor, 0.0f, 1.0f);
            float c = 1.0f - h_clamped;
            int m_top = (ly - 5) + (int)roundf(5.0f * powf(c, 0.75f));
            int m_bot = (ly + 4) - (int)roundf(3.0f * powf(c, 1.50f));
            int m_h = m_bot - m_top + 1;
            if (m_h < 2) m_h = 2;
            int m_r = (m_h <= 2) ? 1 : ((m_h < 6) ? 2 : 3);
            cv.fillRoundRect(lx - 7, m_top, 14, m_h, m_r, TFT_WHITE);
            cv.fillRoundRect(rx - 7, m_top, 14, m_h, m_r, TFT_WHITE);
        }
    }
}

void transitionExpression(Expression fromExpr, Expression toExpr, float durationMs) {
    if (fromExpr == toExpr) return;
    if (!s_canvas_ptr) return;
    clearOcularParticles();

    /* Execute smooth natural blink transition between distinct bitmap expressions */
    int steps = 10;
    float stepDelay = durationMs / (float)steps;

    for (int i = 0; i <= steps; i++) {
        updateGazeSystem();
        float t = (float)i / (float)steps;

        float eyeH;
        Expression activeExpr;
        if (t <= 0.40f) {
            /* Closing down-phase */
            float p = t / 0.40f;
            eyeH = blinkCloseEase(p);
            activeExpr = fromExpr;
        } else if (t <= 0.55f) {
            /* Palpebral closure dwell: seamlessly switch expression while eyelids are shut */
            eyeH = 0.0f;
            activeExpr = toExpr;
        } else {
            /* Opening up-phase */
            float p = (t - 0.55f) / 0.45f;
            eyeH = blinkOpenEase(p);
            activeExpr = toExpr;
        }

        drawFace(activeExpr, eyeH, g_currentOffsetX, g_currentOffsetY, 0.0f, g_currentVergence, g_currentEyeScale);
        vTaskDelay(pdMS_TO_TICKS((int)stepDelay));
    }

    g_currentExpr = toExpr;
    clearOcularParticles();
}
