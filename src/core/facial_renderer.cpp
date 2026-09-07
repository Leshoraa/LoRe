/**
 * @file facial_renderer.cpp
 * @brief Rigid 2D facial rig composition, eye/mouth geometry, and expression transition easing implementation.
 */

#include "src/core/facial_renderer.h"
#include "src/core/display_engine.h"
#include "src/core/gaze_engine.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include "src/math/kinematics.h"
#include "src/math/affective_engine.h"
#include <Arduino.h>
#include <math.h>

static LGFX_Sprite* s_canvas_ptr = &canvas;

void set_facial_canvas(LGFX_Sprite* p_canvas) {
    if (p_canvas) {
        s_canvas_ptr = p_canvas;
    }
}

static void drawDualEyes(float leftHeightFactor, float rightHeightFactor, int ox, int oy, uint16_t color, float vergence, float scale) {
    (void)vergence;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int lx = 32 + ox;
    int rx = 96 + ox;
    int ly = 28 + oy;
    int ry = 28 + oy;

    float sx = (g_currentEyeScaleX > 0.01f) ? (scale * g_currentEyeScaleX) : scale;
    float sy = (g_currentEyeScaleY > 0.01f) ? (scale * g_currentEyeScaleY) : scale;

    int maxEyeWidth = (int)roundf(28.0f * sx);
    int maxEyeHeight = (int)roundf(38.0f * sy);

    int leftHeight = (int)roundf((float)maxEyeHeight * leftHeightFactor);
    int rightHeight = (int)roundf((float)maxEyeHeight * rightHeightFactor);

    if (leftHeight <= 3) {
        cv.fillRoundRect(lx - maxEyeWidth / 2, ly - 1, maxEyeWidth, 3, 1, color);
    } else {
        int radius = (leftHeight < 24) ? leftHeight / 2 : (int)roundf(12.0f * sx);
        if (radius > maxEyeWidth / 2) radius = maxEyeWidth / 2;
        if (radius < 1) radius = 1;
        cv.fillRoundRect(lx - maxEyeWidth / 2, ly - leftHeight / 2, maxEyeWidth, leftHeight, radius, color);
    }

    if (rightHeight <= 3) {
        cv.fillRoundRect(rx - maxEyeWidth / 2, ry - 1, maxEyeWidth, 3, 1, color);
    } else {
        int radius = (rightHeight < 24) ? rightHeight / 2 : (int)roundf(12.0f * sx);
        if (radius > maxEyeWidth / 2) radius = maxEyeWidth / 2;
        if (radius < 1) radius = 1;
        cv.fillRoundRect(rx - maxEyeWidth / 2, ry - rightHeight / 2, maxEyeWidth, rightHeight, radius, color);
    }
}

static void drawEyes(float eyeHeightFactor, int ox, int oy, uint16_t color, float vergence, float scale) {
    drawDualEyes(eyeHeightFactor, eyeHeightFactor, ox, oy, color, vergence, scale);
}

static void drawFumoEye(int cx, int cy, int w, int h, int r, uint16_t color) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    if (h <= 3) {
        cv.fillRoundRect(cx - w / 2, cy - 1, w, 3, 1, color);
        return;
    }
    int topY = cy - h / 2;
    int leftX = cx - w / 2;
    int rad = (r > h) ? h : ((r > w / 2) ? w / 2 : r);
    if (rad < 1) rad = 1;

    if (h > rad) {
        cv.fillRect(leftX, topY, w, h - rad, color);
    }
    cv.fillRoundRect(leftX, topY + h - rad * 2, w, rad * 2, rad, color);
}

static void drawFumoEyes(float eyeHeightFactor, int ox, int oy, uint16_t color, float vergence, float scale) {
    (void)vergence;
    int lx = 32 + ox;
    int rx = 96 + ox;
    int ly = 28 + oy;
    int ry = 28 + oy;

    float sx = (g_currentEyeScaleX > 0.01f) ? (scale * g_currentEyeScaleX) : scale;
    float sy = (g_currentEyeScaleY > 0.01f) ? (scale * g_currentEyeScaleY) : scale;

    int maxEyeWidth = (int)roundf(28.0f * sx);
    int maxEyeHeight = (int)roundf(34.0f * sy);
    int eyeHeight = (int)roundf((float)maxEyeHeight * eyeHeightFactor);
    int topRad = (int)roundf(14.0f * sx);

    drawFumoEye(lx, ly, maxEyeWidth, eyeHeight, topRad, color);
    drawFumoEye(rx, ry, maxEyeWidth, eyeHeight, topRad, color);
}

static void drawJoyEyes(int ox, int oy, float joyScale, uint16_t color, float vergence, float scale) {
    (void)vergence;
    (void)scale;
    if (joyScale <= 0.05f) return;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int lx = 32 + ox;
    int rx = 96 + ox;
    int ly = 31 + oy;

    auto drawThickLine = [&](float x0, float y0, float x1, float y1, int thickness) {
        float dx = x1 - x0;
        float dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        int steps = (int)ceilf(len / 0.5f);
        if (steps < 1) steps = 1;
        for (int i = 0; i <= steps; i++) {
            float t = (float)i / steps;
            int cx = (int)roundf(x0 + dx * t);
            int cy = (int)roundf(y0 + dy * t);
            if (thickness <= 2) {
                cv.fillCircle(cx, cy, 1, color);
            } else {
                int offset = thickness / 2;
                cv.fillRect(cx - offset, cy - offset, thickness, thickness, color);
            }
        }
    };

    float ew = 9.0f * joyScale;
    float eh = 9.0f * joyScale;

    drawThickLine(lx - ew, ly - eh, lx + ew, ly, 4);
    drawThickLine(lx + ew, ly, lx - ew, ly + eh, 4);

    drawThickLine(rx + ew, ly - eh, rx - ew, ly, 4);
    drawThickLine(rx - ew, ly, rx + ew, ly + eh, 4);

    float blx = lx - 22.0f * joyScale;
    float by = ly - 14.0f * joyScale;
    drawThickLine(blx, by, blx + 3.0f * joyScale, by + 5.0f * joyScale, 2);
    drawThickLine(blx + 6.0f * joyScale, by, blx + 9.0f * joyScale, by + 5.0f * joyScale, 2);

    float brx = rx + 14.0f * joyScale;
    drawThickLine(brx, by, brx + 3.0f * joyScale, by + 5.0f * joyScale, 2);
    drawThickLine(brx + 6.0f * joyScale, by, brx + 9.0f * joyScale, by + 5.0f * joyScale, 2);
}

static void drawAngryBrows(float eyeHeightFactor, int ox, int oy, float browAlpha, uint16_t color, float vergence, float scale) {
    (void)vergence;
    (void)scale;
    if (eyeHeightFactor <= 0.15f || browAlpha <= 0.01f) return;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int lx = 32 + ox;
    int rx = 96 + ox;
    int ly = 28 + oy;

    int maxEyeWidth = 28;
    int maxEyeHeight = 38;
    int eyeHeight = (int)roundf((float)maxEyeHeight * eyeHeightFactor);

    int browCutX = (int)((maxEyeWidth / 2 + 4) * browAlpha);
    int browCutY = (int)((eyeHeight / 2 + 3) * browAlpha);
    int eyeTop = ly - eyeHeight / 2;

    cv.fillTriangle(
        lx - 2, eyeTop - 1,
        lx - 2 + browCutX, eyeTop - 1,
        lx - 2 + browCutX, eyeTop - 1 + browCutY,
        color
    );

    cv.fillTriangle(
        rx + 2, eyeTop - 1,
        rx + 2 - browCutX, eyeTop - 1,
        rx + 2 - browCutX, eyeTop - 1 + browCutY,
        color
    );
}

static void drawShockEyes(int ox, int oy, uint16_t color, float vergence, float scale) {
    (void)vergence;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int lx = 32 + ox;
    int rx = 96 + ox;
    int ly = 28 + oy;
    int ry = 28 + oy;

    int w = 28;
    int h = (int)roundf(36.0f * scale);
    if (h <= 3) {
        cv.fillRoundRect(lx - w / 2, ly - 1, w, 3, 1, color);
        cv.fillRoundRect(rx - w / 2, ry - 1, w, 3, 1, color);
        return;
    }
    int rad = (h < 24) ? h / 2 : 12;
    cv.drawRoundRect(lx - w / 2, ly - h / 2, w, h, rad, color);
    if (h > 12) {
        cv.drawRoundRect(lx - w / 2 + 1, ly - h / 2 + 1, w - 2, h - 2, (rad > 1 ? rad - 1 : 1), color);
    }
    int pupilRad = (h > 18) ? 3 : (h > 8 ? 2 : 1);
    cv.fillCircle(lx, ly, pupilRad, color);

    cv.drawRoundRect(rx - w / 2, ry - h / 2, w, h, rad, color);
    if (h > 12) {
        cv.drawRoundRect(rx - w / 2 + 1, ry - h / 2 + 1, w - 2, h - 2, (rad > 1 ? rad - 1 : 1), color);
    }
    cv.fillCircle(rx, ry, pupilRad, color);
}

static void drawSpiralEye(int cx, int cy, float rotAngle, uint16_t color, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    float prevX = cx;
    float prevY = cy;
    for (float theta = 0.2f; theta <= 13.5f; theta += 0.35f) {
        float r = 0.85f * theta * scale;
        float angle = theta + rotAngle;
        float x = cx + r * cosf(angle);
        float y = cy + r * sinf(angle);
        cv.drawLine((int)prevX, (int)prevY, (int)x, (int)y, color);
        prevX = x;
        prevY = y;
    }
}

static void drawSpiralEyes(int ox, int oy, float rotAngle, uint16_t color, float vergence, float scale) {
    (void)vergence;
    int lx = 32 + ox;
    int rx = 96 + ox;
    int ly = 28 + oy;
    int ry = 28 + oy;

    LGFX_Sprite& cv = *s_canvas_ptr;
    if (scale <= 0.05f) {
        cv.fillRoundRect(lx - 14, ly - 1, 28, 3, 1, color);
        cv.fillRoundRect(rx - 14, ry - 1, 28, 3, 1, color);
        return;
    }
    drawSpiralEye(lx, ly, rotAngle, color, scale);
    drawSpiralEye(rx, ry, -rotAngle, color, scale);
}

static void drawSadEyes(int ox, int oy, float animFrame, uint16_t color, float vergence, float scale) {
    (void)vergence;
    LGFX_Sprite& cv = *s_canvas_ptr;

    int lx = 32 + ox;
    int rx = 96 + ox;
    int ly = 25 + oy;
    int ry = 25 + oy;

    int barW = (int)roundf(26.0f * scale);
    int barH = 3;

    cv.fillRoundRect(lx - barW / 2, ly - 1, barW, barH, 1, color);
    cv.fillRoundRect(rx - barW / 2, ry - 1, barW, barH, 1, color);

    int tearOffsets[2] = { -6, 6 };
    int eyesX[2] = { lx, rx };

    for (int e = 0; e < 2; e++) {
        int cx = eyesX[e];
        for (int t = 0; t < 2; t++) {
            int tx = cx + (int)roundf((float)tearOffsets[t] * scale);
            int startY = ly + 2;
            int endY = 58;

            for (int ty = startY; ty <= endY; ty += 2) {
                float phase = ((float)(ty - startY) * 0.35f) - (animFrame * 3.5f);
                float wave = sinf(phase);
                if (wave > -0.65f) {
                    int thickness = (wave > 0.3f) ? 1 : 0;
                    cv.drawFastVLine(tx, ty, 2, color);
                    if (thickness > 0) {
                        cv.drawFastVLine(tx - 1, ty, 2, color);
                    }
                }
            }
        }
    }
}

static void drawMouthCustom(int ox, int oy, float curve, float baseY, float width, float asym, uint16_t color, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    int mx = 64 + ox;
    int my = (int)roundf(baseY + (float)oy);
    float scaledWidth = width * scale;
    for (float x = -scaledWidth; x <= scaledWidth; x += 0.4f) {
        float normX = (scale > 0.01f) ? (x / scale) : x;
        float y = (float)my + (curve * normX * normX + asym * normX) * scale;
        cv.fillCircle(mx + (int)roundf(x), (int)roundf(y), 1, color);
    }
}

static void drawCatMouth(int ox, int oy, uint16_t color, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    int mx = 64 + ox;
    int my = (int)roundf(43.5f + (float)oy);
    float halfW = 8.5f * scale;
    float depth = 3.0f * scale;

    for (float x = -halfW; x <= halfW; x += 0.35f) {
        float normX;
        if (x < 0.0f) {
            normX = (x + halfW * 0.5f) / (halfW * 0.5f);
        } else {
            normX = (x - halfW * 0.5f) / (halfW * 0.5f);
        }
        float y = (float)my + depth * (1.0f - normX * normX);
        cv.fillCircle(mx + (int)roundf(x), (int)roundf(y), 1, color);
    }
}

static void drawDeadpanMouth(int ox, int oy, uint16_t color, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    int mx = 64 + ox;
    int my = (int)roundf(44.0f + (float)oy);
    int halfW = (int)roundf(7.5f * scale);
    cv.fillRoundRect(mx - halfW, my - 1, halfW * 2, 3, 1, color);
}

static void drawJoyMouth(int ox, int oy, float joyScale, uint16_t color, float scale) {
    if (joyScale <= 0.05f) return;
    LGFX_Sprite& cv = *s_canvas_ptr;
    int mx = 64 + ox;
    float width = 12.0f * joyScale * scale; 
    float depth = 6.0f * joyScale * scale;
    int baseY = (int)roundf(38.0f + (float)oy);

    float halfWidth = width / 2.0f;

    for (float x = -width; x <= width; x += 0.3f) {
        float local_x = fabsf(x) - halfWidth;
        float norm = local_x / halfWidth;
        float y = (float)baseY + depth - depth * (norm * norm);
        
        int cx = mx + (int)roundf(x);
        int cy = (int)roundf(y);
        cv.fillRect(cx - 2, cy - 2, 4, 4, color);
    }
}

static void drawShockMouth(int ox, int oy, uint16_t color, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    int mx = 64 + ox;
    int my = (int)roundf(39.0f + (float)oy);
    int w = (int)roundf(16.0f * scale);
    int h = (int)roundf(14.0f * scale);
    cv.fillRoundRect(mx - w / 2, my, w, h, (int)roundf(5.0f * scale), color);
}

static void drawOverloadMouth(int ox, int oy, uint16_t color, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    int mx = 64 + ox;
    int my = (int)roundf(45.0f + (float)oy);
    cv.fillEllipse(mx, my, (int)roundf(7.0f * scale), (int)roundf(5.0f * scale), color);
}

static void drawSadMouth(int ox, int oy, float animFrame, uint16_t color, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    int mx = 64 + ox;
    int my = (int)roundf(45.0f + (float)oy);
    float halfW = 9.5f * scale;
    float waveAmp = 1.8f * scale;
    float waveFreq = 0.75f;
    float phase = animFrame * 4.5f;

    for (float x = -halfW; x <= halfW; x += 0.35f) {
        float norm = x / halfW;
        float taper = 1.0f - norm * norm * norm * norm;
        float y = (float)my + waveAmp * sinf(waveFreq * (x / scale) + phase) * taper;
        cv.fillCircle(mx + (int)roundf(x), (int)roundf(y), 1, color);
    }
}

static inline void drawActiveIndicator(uint16_t color) {
    if (g_recon_state == STATE_ACTIVE) {
        s_canvas_ptr->fillRect(124, 3, 2, 2, color);
    }
}

static void renderFaceState(float eyeHeightFactor, int ox, int oy, float mouthCurve, float mouthY, float mouthWidth, float browAlpha, bool inverted, float vergence, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    uint16_t bgColor = inverted ? TFT_WHITE : TFT_BLACK;
    uint16_t fgColor = inverted ? TFT_BLACK : TFT_WHITE;

    cv.fillScreen(bgColor);
    drawEyes(eyeHeightFactor, ox, oy, fgColor, vergence, scale);
    drawAngryBrows(eyeHeightFactor, ox, oy, browAlpha, bgColor, vergence, scale);
    drawMouthCustom(ox, oy, mouthCurve, mouthY, mouthWidth, 0.0f, fgColor, scale);
    drawActiveIndicator(fgColor);
    cv.pushSprite(0, 0);
}

void drawFace(Expression expr, float eyeHeightFactor, float offsetX, float offsetY, float frame, float vergence, float scale) {
    LGFX_Sprite& cv = *s_canvas_ptr;
    int ox = getFilteredOx(offsetX) + get_burn_shift_x();
    int oy = getFilteredOy(offsetY) + get_burn_shift_y();

    switch (expr) {
        case EXPR_IDLE:
            renderFaceState(eyeHeightFactor, ox, oy, -0.030f, 44.0f, 7.5f, 0.0f, false, vergence, scale);
            break;
        case EXPR_JOY:
            cv.fillScreen(TFT_BLACK);
            drawJoyEyes(ox, oy, 1.0f, TFT_WHITE, vergence, scale);
            drawJoyMouth(ox, oy, 1.0f, TFT_WHITE, scale);
            drawActiveIndicator(TFT_WHITE);
            cv.pushSprite(0, 0);
            break;
        case EXPR_ANGRY:
            renderFaceState(eyeHeightFactor, ox, oy, 0.042f, 43.0f, 7.0f, 1.0f, true, vergence, scale);
            break;
        case EXPR_SMIRK:
            cv.fillScreen(TFT_BLACK);
            drawFumoEyes(eyeHeightFactor, ox, oy, TFT_WHITE, vergence, scale);
            drawCatMouth(ox, oy, TFT_WHITE, scale);
            drawActiveIndicator(TFT_WHITE);
            cv.pushSprite(0, 0);
            break;
        case EXPR_SHOCK:
            cv.fillScreen(TFT_BLACK);
            if (eyeHeightFactor > 0.3f) {
                drawShockEyes(ox, oy, TFT_WHITE, vergence, scale);
            } else {
                drawEyes(eyeHeightFactor, ox, oy, TFT_WHITE, vergence, scale);
            }
            drawShockMouth(ox, oy, TFT_WHITE, scale);
            drawActiveIndicator(TFT_WHITE);
            cv.pushSprite(0, 0);
            break;
        case EXPR_OVERLOAD:
            cv.fillScreen(TFT_BLACK);
            drawSpiralEyes(ox, oy, frame, TFT_WHITE, vergence, scale);
            drawOverloadMouth(ox, oy, TFT_WHITE, scale);
            drawActiveIndicator(TFT_WHITE);
            cv.pushSprite(0, 0);
            break;
        case EXPR_SAD:
            cv.fillScreen(TFT_BLACK);
            drawSadEyes(ox, oy, frame, TFT_WHITE, vergence, scale);
            drawSadMouth(ox, oy, frame, TFT_WHITE, scale);
            drawActiveIndicator(TFT_WHITE);
            cv.pushSprite(0, 0);
            break;
        case EXPR_DEADPAN:
            cv.fillScreen(TFT_BLACK);
            drawFumoEyes(eyeHeightFactor, ox, oy, TFT_WHITE, vergence, scale);
            drawDeadpanMouth(ox, oy, TFT_WHITE, scale);
            drawActiveIndicator(TFT_WHITE);
            cv.pushSprite(0, 0);
            break;
    }
}

void transitionExpression(Expression fromExpr, Expression toExpr, float durationMs) {
    if (fromExpr == toExpr) return;
    LGFX_Sprite& cv = *s_canvas_ptr;

    float startLeftEyeH  = 1.0f;
    float endLeftEyeH    = 1.0f;
    float startRightEyeH = 1.0f;
    float endRightEyeH   = 1.0f;

    float startCurve = (fromExpr == EXPR_IDLE) ? -0.030f : ((fromExpr == EXPR_ANGRY) ? 0.042f : 0.0f);
    float endCurve   = (toExpr == EXPR_IDLE)   ? -0.030f : ((toExpr == EXPR_ANGRY)   ? 0.042f : 0.0f);

    float startY = (fromExpr == EXPR_IDLE) ? 44.0f : ((fromExpr == EXPR_ANGRY) ? 43.0f : ((fromExpr == EXPR_SHOCK) ? 39.0f : ((fromExpr == EXPR_OVERLOAD) ? 45.0f : 44.0f)));
    float endY   = (toExpr == EXPR_IDLE)   ? 44.0f : ((toExpr == EXPR_ANGRY)   ? 43.0f : ((toExpr == EXPR_SHOCK)   ? 39.0f : ((toExpr == EXPR_OVERLOAD)   ? 45.0f : 44.0f)));

    float startW = (fromExpr == EXPR_IDLE) ? 7.5f : ((fromExpr == EXPR_ANGRY) ? 7.0f : 8.0f);
    float endW   = (toExpr == EXPR_IDLE)   ? 7.5f : ((toExpr == EXPR_ANGRY)   ? 7.0f : 8.0f);

    float startAsym = 0.0f;
    float endAsym   = 0.0f;

    float startBrow = (fromExpr == EXPR_ANGRY) ? 1.0f : 0.0f;
    float endBrow   = (toExpr == EXPR_ANGRY)   ? 1.0f : 0.0f;

    int steps = 14;
    float stepDelay = durationMs / (float)steps;
    float arousal = getEmotionArousal();

    for (int i = 0; i <= steps; i++) {
        updateGazeSystem();

        float t = (float)i / (float)steps;

        float curLeftEyeH;
        float curRightEyeH;
        if (t <= 0.35f) {
            float p = t / 0.35f;
            float blinkFactor = fmaxf(0.04f, 1.0f - p * p);
            curLeftEyeH = startLeftEyeH * blinkFactor;
            curRightEyeH = startRightEyeH * blinkFactor;
        } else if (t <= 0.50f) {
            curLeftEyeH = 0.04f;
            curRightEyeH = 0.04f;
        } else {
            float p = (t - 0.50f) / 0.50f;
            float blinkFactor = fmaxf(0.04f, blinkOpenEase(p));
            curLeftEyeH = endLeftEyeH * blinkFactor;
            curRightEyeH = endRightEyeH * blinkFactor;
        }

        float elasticT = eval_elastic_bounce_ease(t);
        float curCurve = customLerp(startCurve, endCurve, elasticT);
        float curY     = customLerp(startY, endY, elasticT);
        float curW     = customLerp(startW, endW, elasticT);
        float curAsym  = customLerp(startAsym, endAsym, elasticT);
        float curBrow  = customLerp(startBrow, endBrow, elasticT);

        float squashX = 1.0f, squashY = 1.0f;
        compute_squash_stretch_factors(t, arousal, &squashX, &squashY);
        g_currentEyeScaleX = squashX;
        g_currentEyeScaleY = squashY;

        float joyScale = (t < 0.5f) ? fmaxf(0.0f, 1.0f - t * 2.0f) : fmaxf(0.0f, (t - 0.5f) * 2.0f);
        joyScale *= squashY;

        bool inverted = (t < 0.5f) ? (fromExpr == EXPR_ANGRY) : (toExpr == EXPR_ANGRY);
        uint16_t bgColor = inverted ? TFT_WHITE : TFT_BLACK;
        uint16_t fgColor = inverted ? TFT_BLACK : TFT_WHITE;

        int ox = getFilteredOx(g_currentOffsetX) + get_burn_shift_x();
        int oy = getFilteredOy(g_currentOffsetY) + get_burn_shift_y();

        cv.fillScreen(bgColor);

        Expression activeExpr = (t < 0.5f) ? fromExpr : toExpr;
        if (activeExpr == EXPR_IDLE || activeExpr == EXPR_ANGRY) {
            drawDualEyes(curLeftEyeH, curRightEyeH, ox, oy, fgColor, g_currentVergence, g_currentEyeScale);
        } else if (activeExpr == EXPR_SMIRK || activeExpr == EXPR_DEADPAN) {
            drawFumoEyes(curLeftEyeH, ox, oy, fgColor, g_currentVergence, g_currentEyeScale);
        } else if (activeExpr == EXPR_JOY) {
            drawJoyEyes(ox, oy, joyScale, fgColor, g_currentVergence, g_currentEyeScale);
        } else if (activeExpr == EXPR_SHOCK) {
            drawShockEyes(ox, oy, fgColor, g_currentVergence, g_currentEyeScale * curLeftEyeH * squashY);
        } else if (activeExpr == EXPR_OVERLOAD) {
            drawSpiralEyes(ox, oy, t * 2.0f, fgColor, g_currentVergence, g_currentEyeScale * curLeftEyeH * squashY);
        } else if (activeExpr == EXPR_SAD) {
            drawSadEyes(ox, oy, t * 4.0f, fgColor, g_currentVergence, g_currentEyeScale * curLeftEyeH * squashY);
        }

        if (curBrow > 0.01f) {
            drawAngryBrows(curLeftEyeH, ox, oy, curBrow, bgColor, g_currentVergence, g_currentEyeScale);
        }

        bool fromCurveMouth = (fromExpr == EXPR_IDLE || fromExpr == EXPR_ANGRY);
        bool toCurveMouth   = (toExpr == EXPR_IDLE || toExpr == EXPR_ANGRY);

        if (fromCurveMouth && toCurveMouth) {
            drawMouthCustom(ox, oy, curCurve, curY, curW, curAsym, fgColor, g_currentEyeScale * squashX);
        } else {
            Expression mouthExpr = (t < 0.5f) ? fromExpr : toExpr;
            if (mouthExpr == EXPR_IDLE || mouthExpr == EXPR_ANGRY) {
                drawMouthCustom(ox, oy, curCurve, curY, curW, curAsym, fgColor, g_currentEyeScale * squashX);
            } else if (mouthExpr == EXPR_SMIRK) {
                drawCatMouth(ox, oy, fgColor, g_currentEyeScale * squashX);
            } else if (mouthExpr == EXPR_DEADPAN) {
                drawDeadpanMouth(ox, oy, fgColor, g_currentEyeScale * squashX);
            } else if (mouthExpr == EXPR_JOY) {
                drawJoyMouth(ox, oy, joyScale, fgColor, g_currentEyeScale);
            } else if (mouthExpr == EXPR_SHOCK) {
                drawShockMouth(ox, oy, fgColor, g_currentEyeScale * squashY);
            } else if (mouthExpr == EXPR_OVERLOAD) {
                drawOverloadMouth(ox, oy, fgColor, g_currentEyeScale * squashX);
            } else if (mouthExpr == EXPR_SAD) {
                drawSadMouth(ox, oy, t * 4.0f, fgColor, g_currentEyeScale * squashX);
            }
        }

        drawActiveIndicator(fgColor);
        cv.pushSprite(0, 0);
        vTaskDelay(pdMS_TO_TICKS((int)stepDelay));
    }
    g_currentEyeScaleX = 1.0f;
    g_currentEyeScaleY = 1.0f;
    g_currentExpr = toExpr;
}
