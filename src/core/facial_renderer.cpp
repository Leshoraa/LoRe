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
#include <Arduino.h>
#include <math.h>

static LGFX_Sprite* s_canvas_ptr = &canvas;

void set_facial_canvas(LGFX_Sprite* p_canvas) {
    if (p_canvas) {
        s_canvas_ptr = p_canvas;
    }
}

static void renderOneEyeSuperellipse(LGFX_Sprite& cv, float xc, float yc, float a, float b, float n, float theta, float stroke) {
    if (b <= 1.5f) {
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

void drawAutonomousSoma(const OcularSomaState& soma, float offsetX, float offsetY, float palpebralAperture) {
    if (!s_canvas_ptr) return;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int ox = getFilteredOx(offsetX) + get_burn_shift_x();
    int oy = getFilteredOy(offsetY) + get_burn_shift_y();

    cv.fillScreen(TFT_BLACK);

    float aperture = constrain(palpebralAperture, 0.0f, 1.0f);

    float left_xc = soma.left_x + (float)ox + soma.nystagmus_x;
    float left_yc = soma.left_y + (float)oy + soma.nystagmus_y;
    float right_xc = soma.right_x + (float)ox + soma.nystagmus_x;
    float right_yc = soma.right_y + (float)oy + soma.nystagmus_y;

    float left_a = soma.left_w * 0.5f;
    float left_b = soma.left_h * 0.5f * aperture;
    float right_a = soma.right_w * 0.5f;
    float right_b = soma.right_h * 0.5f * aperture;

    renderOneEyeSuperellipse(cv, left_xc, left_yc, left_a, left_b, soma.left_n, soma.tilt_left, soma.stroke_thickness);
    renderOneEyeSuperellipse(cv, right_xc, right_yc, right_a, right_b, soma.right_n, soma.tilt_right, soma.stroke_thickness);

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
                break;

            case EXPR_IDLE:
            default:
                drawAutonomousSoma(getOcularSomaState(), offsetX, offsetY, h_clamped);
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
}
