/**
 * @file kinematics.cpp
 * @brief Biomechanical ocular kinematics, mass-spring-damper, and minimum-jerk solver implementation.
 */

#include "src/math/kinematics.h"
#include "src/ai/personality_engine.h"
#include "src/ai/brain_engine.h"
#include "src/math/affective_engine.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include <math.h>
#include <Arduino.h>
#include <esp_random.h>

/* Global Kinematic State */
float g_currentOffsetX = 0.0f;
float g_currentOffsetY = 0.0f;
float g_currentVergence = 0.0f;
float g_currentEyeScale = 1.0f;
float g_currentEyeScaleX = 1.0f;
float g_currentEyeScaleY = 1.0f;
bool g_is_transitioning = false;

/* Internal Dynamic State */
static float s_eye_vx = 0.0f;
static float s_eye_vy = 0.0f;
static float s_smoothedTargetX = 0.0f;
static float s_smoothedTargetY = 0.0f;
static bool s_trackInSaccade = false;
static uint32_t s_trackSaccadeStart = 0;
static uint32_t s_trackSaccadeDuration = 60;
static float s_trackSaccadeStartX = 0.0f;
static float s_trackSaccadeStartY = 0.0f;
static float s_trackSaccadeTargetX = 0.0f;
static float s_trackSaccadeTargetY = 0.0f;

static float s_startOffsetX = 0.0f;
static float s_startOffsetY = 0.0f;
static float s_targetOffsetX = 0.0f;
static float s_targetOffsetY = 0.0f;
static unsigned long s_gazeStartTime = 0;
static unsigned long s_gazeDuration = 120;
static unsigned long s_nextGazeTime = 0;
static bool s_inSaccade = false;

/* Fixational Microsaccade State & Constants */
static uint32_t s_nextMicrosaccadeTime = 0;
static bool s_inMicrosaccade = false;
static uint32_t s_microsaccadeStartTime = 0;
static float s_microsaccadeStartX = 0.0f;
static float s_microsaccadeStartY = 0.0f;
static float s_microsaccadeTargetX = 0.0f;
static float s_microsaccadeTargetY = 0.0f;

static const float kOUMeanReversionRate = 2.8f;      /* Mean-reversion pull toward gaze anchor */
static const float kOUVolatilitySigma = 0.18f;        /* Stochastic diffusion amplitude */
static const uint32_t kMicrosaccadeDurationMs = 28;  /* Rapid biological flick duration */
static const float kMicrosaccadeThresholdPx = 0.25f;  /* Retinal drift error threshold */
static const uint32_t kMicrosaccadeIntervalMinMs = 1200;
static const uint32_t kMicrosaccadeIntervalRangeMs = 1300;

/* Lévy Flight Heavy-Tailed Gaze Exploration Constants */
static const float kLevyWideBaseProb = 0.05f;
static const float kLevyWideCuriosityGain = 0.12f;
static const float kLevyMedBaseProb = 0.20f;
static const float kLevyMedCuriosityGain = 0.05f;
static const float kLevyLocalMinRadiusPx = 0.5f;
static const float kLevyLocalMaxRadiusPx = 3.5f;
static const float kLevyMediumMinRadiusPx = 4.0f;
static const float kLevyMediumMaxRadiusPx = 8.5f;
static const float kLevyWideMinRadiusPx = 9.5f;
static const float kLevyWideMaxRadiusPx = 15.0f;

/* Post-Saccadic Ocular Glissade Constants (Extraocular Soft-Tissue Compliance) */
static const float kGlissadeOnsetTau = 0.70f;
static const float kGlissadeDecayLambda = 3.5f;
static const float kGlissadeMaxOvershoot = 1.06f;

/* Affective Saccade Kinematics Modulation Table */
static const AffectiveKinematicProfile kAffectiveKinematicTable[NUM_EXPRESSIONS] = {
    /* 0: EXPR_IDLE - Baseline balanced organic kinematics */
    { 1.00f, 1.00f, 1.00f, 0.045f },
    /* 1: EXPR_HAPPY - Springy, light, cheerful bounce */
    { 1.15f, 0.82f, 0.88f, 0.065f },
    /* 2: EXPR_ANGRY - Hyper-fast, aggressive snap, dead-stop rigid settling */
    { 1.40f, 1.30f, 0.70f, 0.005f },
    /* 3: EXPR_SAD - Sluggish, dejected drag, low velocity */
    { 0.75f, 1.10f, 1.45f, 0.020f },
    /* 4: EXPR_SURPRISED - Instantaneous startle snap, rapid fixation lock */
    { 1.50f, 1.00f, 0.65f, 0.030f },
    /* 5: EXPR_SUSPICIOUS - Controlled, guarded, scrutinizing glance */
    { 1.10f, 1.25f, 1.15f, 0.010f },
    /* 6: EXPR_CURIOUS - Inquisitive darting flicks */
    { 1.20f, 0.90f, 0.85f, 0.055f },
    /* 7: EXPR_MISCHIEF - Playful, cheeky overshoot */
    { 1.25f, 0.78f, 0.80f, 0.070f },
    /* 8: EXPR_SLEEPY - Heavily damped, slow viscous glide */
    { 0.65f, 1.20f, 1.60f, 0.015f },
    /* 9: EXPR_COOL - Smooth, relaxed, effortless swagger */
    { 0.95f, 1.05f, 1.10f, 0.035f },
    /* 10: EXPR_DIZZY - Uncoordinated wobbling glide */
    { 0.80f, 0.70f, 1.30f, 0.080f },
    /* 11: EXPR_CRYING - Trembling, sorrowful gaze shifts */
    { 0.85f, 0.95f, 1.35f, 0.040f }
};

static float s_active_omega_mult = 1.0f;
static float s_active_zeta_mult = 1.0f;
static float s_active_dur_mult = 1.0f;
static float s_active_glissade = 0.045f;

static bool s_prevTargetDetected = false;
static float s_deadbandTargetX = 0.0f;
static float s_deadbandTargetY = 0.0f;
static bool s_hasTargetLock = false;

/* Coordinate Hysteresis Filter State */
static float s_stable_ox = 0.0f;
static float s_stable_oy = 0.0f;

float eval_minimum_jerk_spline(float p) {
    if (p <= 0.0f) return 0.0f;
    if (p >= 1.0f) return 1.0f;
    float base_spline = p * p * p * (10.0f + p * (-15.0f + 6.0f * p));
    /* Post-saccadic ocular glissade rebound upon target landing */
    if (p > kGlissadeOnsetTau) {
        float delta_tau = p - kGlissadeOnsetTau;
        float norm_tail = delta_tau / (1.0f - kGlissadeOnsetTau);
        float glissade = s_active_glissade * sinf(norm_tail * 3.14159265f) * expf(-kGlissadeDecayLambda * delta_tau);
        base_spline += glissade;
    }
    return constrain(base_spline, 0.0f, kGlissadeMaxOvershoot);
}

uint32_t compute_saccade_duration_ms(float displacement_px) {
    float deg = fabsf(displacement_px) * PX_TO_DEG_FACTOR;
    float dur = (SACCADE_D0_MS + SACCADE_K_MS_PER_DEG * deg) * s_active_dur_mult;
    if (dur < 75.0f) dur = 75.0f;
    if (dur > 380.0f) dur = 380.0f;
    return (uint32_t)dur;
}

float easeInOutCubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = (-2.0f * t + 2.0f);
        return 1.0f - (f * f * f) / 2.0f;
    }
}

float eval_elastic_bounce_ease(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    /* Analytical Kelvin-Voigt underdamped transient compliance (zeta = 0.72, omega = 7.2 rad/s) */
    const float zeta = 0.72f;
    const float omega = 7.2f;
    const float omega_d = 4.996f; // omega * sqrt(1 - zeta^2)
    const float decay = expf(-zeta * omega * t);
    float response = 1.0f - decay * (cosf(omega_d * t) + (zeta / 0.6939f) * sinf(omega_d * t));
    return response;
}

void compute_squash_stretch_factors(float progress, float arousal, float* scaleX, float* scaleY) {
    if (!scaleX || !scaleY) return;
    if (progress <= 0.0f || progress >= 1.0f) {
        *scaleX = 1.0f;
        *scaleY = 1.0f;
        return;
    }
    /* Biological incompressibility volume conservation: Sy * Sx^2 ≈ 1.0 -> Sx = 1.0 / sqrt(Sy) */
    float effective_arousal = constrain(arousal, 0.20f, 1.0f);
    float bounce_amp = BOUNCE_SQUASH_STRETCH_GAIN * effective_arousal;
    float phase = progress * 6.2831853f; // 2 * PI
    float decay = expf(-3.2f * progress);
    float delta_y = bounce_amp * sinf(phase) * decay;
    
    float sy = constrain(1.0f + delta_y, 0.80f, 1.25f);
    float sx = constrain(1.0f / sqrtf(sy), 0.80f, 1.25f);
    
    *scaleX = sx;
    *scaleY = sy;
}

float blinkCloseEase(float t) {
    if (t <= 0.0f) return 1.0f;
    if (t >= 1.0f) return 0.0f;
    /* Human orbicularis oculi fast-twitch down-phase:
     * Rapid acceleration with peak downward velocity around tau = 0.38 - 0.40,
     * followed by smooth deceleration settling into palpebral contact without hard impact. */
    float u = powf(t, 0.85f);
    float s = u * u * u * (10.0f + u * (-15.0f + 6.0f * u));
    return constrain(1.0f - s, 0.0f, 1.0f);
}

float blinkOpenEase(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    /* Human levator palpebrae superioris viscoelastic up-phase:
     * Smooth ease-out retraction against tissue resistance with subtle
     * 3.5% myogenic settling near resting palpebral aperture. */
    float base = sinf(t * 1.5707963f);
    float settling = 0.035f * sinf(t * 3.14159265f) * expf(-2.5f * (1.0f - t));
    return constrain(base + settling, 0.0f, 1.04f);
}

float customLerp(float a, float b, float t) {
    return a + t * (b - a);
}

int getFilteredOx(float rawOffsetX) {
    if (fabsf(rawOffsetX - s_stable_ox) >= 0.55f) {
        s_stable_ox = roundf(rawOffsetX);
    }
    return (int)s_stable_ox;
}

int getFilteredOy(float rawOffsetY) {
    if (fabsf(rawOffsetY - s_stable_oy) >= 0.55f) {
        s_stable_oy = roundf(rawOffsetY);
    }
    return (int)s_stable_oy;
}

void resetHysteresisFilter(void) {
    s_stable_ox = 0.0f;
    s_stable_oy = 0.0f;
}

void updateGazeSystem(void) {
    TrackTarget target;
    portENTER_CRITICAL(&g_target_mutex);
    target = g_current_target;
    portEXIT_CRITICAL(&g_target_mutex);

    unsigned long now = millis();
    static uint32_t lastGazeTimeUs = 0;
    uint32_t nowUs = micros();
    float dt = (lastGazeTimeUs > 0) ? (float)(nowUs - lastGazeTimeUs) * 0.000001f : 0.016666f;
    if (dt < 0.005f) dt = 0.005f;
    if (dt > 0.040f) dt = 0.040f;
    lastGazeTimeUs = nowUs;

    /* Affective Saccade Kinematics Smooth Interpolation (Rate ~12.0 rad/s) */
    Expression curExpr = g_currentExpr;
    if ((int)curExpr >= 0 && (int)curExpr < NUM_EXPRESSIONS) {
        const AffectiveKinematicProfile* targetProf = &kAffectiveKinematicTable[curExpr];
        float alpha_k = 1.0f - expf(-12.0f * dt);
        s_active_omega_mult += (targetProf->omega_mult - s_active_omega_mult) * alpha_k;
        s_active_zeta_mult += (targetProf->zeta_mult - s_active_zeta_mult) * alpha_k;
        s_active_dur_mult += (targetProf->duration_mult - s_active_dur_mult) * alpha_k;
        s_active_glissade += (targetProf->glissade_gain - s_active_glissade) * alpha_k;
    }

    bool targetActive = (g_recon_state == STATE_ACTIVE) && (target.detected || ((now - target.last_seen_ms) < 300 && target.last_seen_ms > 0));

    /* Continuous Stereoscopic Ocular Vergence (3D Depth Focus) */
    float targetVergence = 0.0f;
    if (targetActive && target.proximity > 0.05f) {
        targetVergence = constrain(target.proximity * 3.2f, 0.0f, 3.5f);
    }
    float alpha_v = 1.0f - expf(-10.0f * dt);
    g_currentVergence += (targetVergence - g_currentVergence) * alpha_v;
    if (fabsf(g_currentVergence) < 0.02f) {
        g_currentVergence = 0.0f;
    }

    /* Volume-Conserving Biological Tissue Squash & Stretch */
    float arousal = getEmotionArousal();
    float sleep_pressure = getBiologicalSleepPressure();
    float targetScaleY = 1.0f + (0.15f * (arousal - 0.20f) - 0.08f * sleep_pressure);
    targetScaleY = constrain(targetScaleY, 0.88f, 1.18f);
    float targetScaleX = 1.0f / sqrtf(targetScaleY);

    float alpha_s = 1.0f - expf(-8.0f * dt);
    g_currentEyeScaleY += (targetScaleY - g_currentEyeScaleY) * alpha_s;
    g_currentEyeScaleX += (targetScaleX - g_currentEyeScaleX) * alpha_s;

    if (fabsf(g_currentEyeScaleY - 1.0f) < 0.005f) {
        g_currentEyeScaleY = 1.0f;
    }
    if (fabsf(g_currentEyeScaleX - 1.0f) < 0.005f) {
        g_currentEyeScaleX = 1.0f;
    }
    g_currentEyeScale = g_currentEyeScaleY;

    if (g_is_transitioning) {
        if (targetActive) {
            float normX = constrain((target.error_x / 100.0f) * GAZE_GAIN_X, -1.0f, 1.0f);
            float normY = constrain((target.error_y / 100.0f) * GAZE_GAIN_Y, -1.0f, 1.0f);
            float rawTargetX = normX * 22.0f;
            float rawTargetY = normY * 14.0f;

            if (!s_hasTargetLock || !s_prevTargetDetected) {
                s_deadbandTargetX = rawTargetX;
                s_deadbandTargetY = rawTargetY;
                s_hasTargetLock = true;
                s_prevTargetDetected = true;
            } else {
                float deltaX = rawTargetX - s_deadbandTargetX;
                float deltaY = rawTargetY - s_deadbandTargetY;
                float deltaDist = sqrtf(deltaX * deltaX + deltaY * deltaY);
                if (deltaDist > GAZE_DEADBAND_RADIUS_PX) {
                    float excess = (deltaDist - GAZE_DEADBAND_RADIUS_PX) / (deltaDist + 1e-6f);
                    s_deadbandTargetX += deltaX * excess * 0.40f;
                    s_deadbandTargetY += deltaY * excess * 0.40f;
                }
            }

            float alpha = 1.0f - expf(-20.0f * dt);
            s_smoothedTargetX += (s_deadbandTargetX - s_smoothedTargetX) * alpha;
            s_smoothedTargetY += (s_deadbandTargetY - s_smoothedTargetY) * alpha;
        }
        s_eye_vx = 0.0f;
        s_eye_vy = 0.0f;
        s_trackInSaccade = false;
        s_inSaccade = false;
        return;
    }

    targetActive = (g_recon_state == STATE_ACTIVE) && target.detected;
    if (targetActive) {
        float normX = constrain((target.error_x / 100.0f) * GAZE_GAIN_X, -1.0f, 1.0f);
        float normY = constrain((target.error_y / 100.0f) * GAZE_GAIN_Y, -1.0f, 1.0f);
        float rawTargetX = normX * 22.0f;
        float rawTargetY = normY * 14.0f;

        if (!s_hasTargetLock || !s_prevTargetDetected) {
            s_deadbandTargetX = rawTargetX;
            s_deadbandTargetY = rawTargetY;
            s_hasTargetLock = true;
        } else {
            float deltaX = rawTargetX - s_deadbandTargetX;
            float deltaY = rawTargetY - s_deadbandTargetY;
            float deltaDist = sqrtf(deltaX * deltaX + deltaY * deltaY);
            if (deltaDist > GAZE_DEADBAND_RADIUS_PX) {
                float excess = (deltaDist - GAZE_DEADBAND_RADIUS_PX) / (deltaDist + 1e-6f);
                s_deadbandTargetX += deltaX * excess * 0.40f;
                s_deadbandTargetY += deltaY * excess * 0.40f;
            }
        }

        float effectiveTargetX = s_deadbandTargetX;
        float effectiveTargetY = s_deadbandTargetY;

        if (!s_prevTargetDetected) {
            s_prevTargetDetected = true;
            float dist_init = sqrtf((effectiveTargetX - g_currentOffsetX) * (effectiveTargetX - g_currentOffsetX) +
                                    (effectiveTargetY - g_currentOffsetY) * (effectiveTargetY - g_currentOffsetY));
            if (dist_init > 10.0f) {
                s_trackInSaccade = true;
                s_trackSaccadeStart = now;
                s_trackSaccadeStartX = g_currentOffsetX;
                s_trackSaccadeStartY = g_currentOffsetY;
                s_trackSaccadeTargetX = effectiveTargetX;
                s_trackSaccadeTargetY = effectiveTargetY;
                s_trackSaccadeDuration = compute_saccade_duration_ms(dist_init);
            } else {
                s_trackInSaccade = false;
            }
            s_smoothedTargetX = g_currentOffsetX;
            s_smoothedTargetY = g_currentOffsetY;
            s_eye_vx = 0.0f;
            s_eye_vy = 0.0f;
            s_inSaccade = false;
        }

        float alpha = 1.0f - expf(-20.0f * dt);
        s_smoothedTargetX += (effectiveTargetX - s_smoothedTargetX) * alpha;
        s_smoothedTargetY += (effectiveTargetY - s_smoothedTargetY) * alpha;

        float dx_eye = effectiveTargetX - g_currentOffsetX;
        float dy_eye = effectiveTargetY - g_currentOffsetY;
        float dist_eye = sqrtf(dx_eye * dx_eye + dy_eye * dy_eye);

        if (dist_eye > 15.0f && !s_trackInSaccade) {
            s_trackInSaccade = true;
            s_trackSaccadeStart = now;
            s_trackSaccadeDuration = compute_saccade_duration_ms(dist_eye);
            s_trackSaccadeStartX = g_currentOffsetX;
            s_trackSaccadeStartY = g_currentOffsetY;
            s_trackSaccadeTargetX = effectiveTargetX;
            s_trackSaccadeTargetY = effectiveTargetY;
            s_eye_vx = 0.0f;
            s_eye_vy = 0.0f;
        }

        if (s_trackInSaccade) {
            float elapsed = (float)(now - s_trackSaccadeStart);
            float progress = elapsed / (float)s_trackSaccadeDuration;

            if (progress >= 1.0f) {
                g_currentOffsetX = s_trackSaccadeTargetX;
                g_currentOffsetY = s_trackSaccadeTargetY;
                s_smoothedTargetX = s_trackSaccadeTargetX;
                s_smoothedTargetY = s_trackSaccadeTargetY;
                s_trackInSaccade = false;
            } else {
                float s = eval_minimum_jerk_spline(progress);
                float distX = s_trackSaccadeTargetX - s_trackSaccadeStartX;
                float distY = s_trackSaccadeTargetY - s_trackSaccadeStartY;
                g_currentOffsetX = s_trackSaccadeStartX + (distX * s);
                g_currentOffsetY = s_trackSaccadeStartY + (distY * s);
            }
        } else {
            /* Personality- and Affective-modulated spring-damper parameters */
            float omega_n = getPersonalityGazeOmega() * s_active_omega_mult;
            float zeta = getPersonalityGazeDamping() * s_active_zeta_mult;

            float ax = (omega_n * omega_n) * (s_smoothedTargetX - g_currentOffsetX) - (2.0f * zeta * omega_n) * s_eye_vx;
            float ay = (omega_n * omega_n) * (s_smoothedTargetY - g_currentOffsetY) - (2.0f * zeta * omega_n) * s_eye_vy;

            s_eye_vx += ax * dt;
            s_eye_vy += ay * dt;

            if (fabsf(s_smoothedTargetX - g_currentOffsetX) < 0.30f && fabsf(s_eye_vx) < 1.2f) {
                s_eye_vx *= 0.60f;
            }
            if (fabsf(s_smoothedTargetY - g_currentOffsetY) < 0.30f && fabsf(s_eye_vy) < 1.2f) {
                s_eye_vy *= 0.60f;
            }

            g_currentOffsetX += s_eye_vx * dt;
            g_currentOffsetY += s_eye_vy * dt;
        }

        g_currentOffsetX = constrain(g_currentOffsetX, -17.5f, 17.5f);
        g_currentOffsetY = constrain(g_currentOffsetY, -12.0f, 11.0f);

        s_inSaccade = false;
        s_nextGazeTime = now + 800;
        return;
    }

    /* On target loss: smoothly ease eyes back to center before resuming calm idle exploration */
    if (s_prevTargetDetected) {
        s_prevTargetDetected = false;
        s_hasTargetLock = false;
        s_trackInSaccade = false;
        s_inSaccade = false;
        s_eye_vx = 0.0f;
        s_eye_vy = 0.0f;
        s_nextGazeTime = now + (uint32_t)(esp_random() % 1500 + 2500);
    }

    bool isSleep = (g_recon_state == STATE_SLEEP_RECON);
    float y_bias = getPersonalityIdleGazeYBias();

    if (!s_inSaccade && !g_is_transitioning && now >= s_nextGazeTime) {
        s_startOffsetX = g_currentOffsetX;
        s_startOffsetY = g_currentOffsetY;

        if (isSleep) {
            uint32_t pick = esp_random() % 100;
            float distFromCenter = sqrtf(s_startOffsetX * s_startOffsetX + s_startOffsetY * s_startOffsetY);

            if (pick < 35 && distFromCenter >= 3.0f) {
                s_targetOffsetX = ((float)(esp_random() % 20) - 10.0f) * 0.1f;
                s_targetOffsetY = ((float)(esp_random() % 16) - 8.0f) * 0.1f;
            } else if (pick < 75) {
                float signX = (s_startOffsetX > 1.0f) ? -1.0f : ((s_startOffsetX < -1.0f) ? 1.0f : ((esp_random() % 2 == 0) ? -1.0f : 1.0f));
                s_targetOffsetX = signX * (3.5f + (float)(esp_random() % 500) * 0.01f);
                s_targetOffsetY = ((float)(esp_random() % 400) - 200.0f) * 0.01f;
            } else {
                float signX = (esp_random() % 2 == 0) ? -1.0f : 1.0f;
                float signY = (esp_random() % 2 == 0) ? -1.0f : 1.0f;
                s_targetOffsetX = signX * (3.0f + (float)(esp_random() % 400) * 0.01f);
                s_targetOffsetY = signY * (2.0f + (float)(esp_random() % 300) * 0.01f);
            }

            s_targetOffsetX = constrain(s_targetOffsetX, -12.0f, 12.0f);
            s_targetOffsetY = constrain(s_targetOffsetY, -8.0f, 7.0f);

            float ds = sqrtf((s_targetOffsetX - s_startOffsetX) * (s_targetOffsetX - s_startOffsetX) +
                             (s_targetOffsetY - s_startOffsetY) * (s_targetOffsetY - s_startOffsetY));
            s_gazeDuration = compute_saccade_duration_ms(ds);
            s_nextGazeTime = now + s_gazeDuration + (esp_random() % 2000 + 3000);
            s_gazeStartTime = now;
            s_inSaccade = true;
        } else {
            /* Weather and circadian context modulation */
            WeatherInfo local_weather;
            portENTER_CRITICAL(&g_weather_mutex);
            local_weather = g_weather_info;
            portEXIT_CRITICAL(&g_weather_mutex);

            bool is_rain = local_weather.valid && ((local_weather.weather_code >= 51 && local_weather.weather_code <= 67) ||
                                                   (local_weather.weather_code >= 80 && local_weather.weather_code <= 82) ||
                                                   (local_weather.weather_code >= 95 && local_weather.weather_code <= 99));

            if (isCircadianSleepTime()) {
                y_bias += 2.5f; /* Resting downward ocular bias during sleep */
            }

            uint32_t pick = esp_random() % 100;
            if (is_rain && (pick < 26)) {
                /* Environmental reflex: gaze gently upward toward sky/ceiling during rain */
                s_targetOffsetX = ((float)(esp_random() % 40) - 20.0f) * 0.1f;
                s_targetOffsetY = -5.0f - (float)(esp_random() % 35) * 0.1f;
            } else {
                /* Lévy Flight Free Exploration: heavy-tailed step distribution modulated by curiosity */
                float curiosity = getBrainCuriosityDrive();
                float p_wide = kLevyWideBaseProb + kLevyWideCuriosityGain * curiosity;
                float p_med = kLevyMedBaseProb + kLevyMedCuriosityGain * curiosity;
                float roll = (float)(esp_random() % 1000) * 0.001f;

                float step_radius = 0.0f;
                if (roll < p_wide) {
                    /* Long-range exploratory peripheral saccade */
                    step_radius = kLevyWideMinRadiusPx + ((float)(esp_random() % 1000) * 0.001f) * (kLevyWideMaxRadiusPx - kLevyWideMinRadiusPx);
                } else if (roll < (p_wide + p_med)) {
                    /* Intermediate focal shift */
                    step_radius = kLevyMediumMinRadiusPx + ((float)(esp_random() % 1000) * 0.001f) * (kLevyMediumMaxRadiusPx - kLevyMediumMinRadiusPx);
                } else {
                    /* Dense local inspection cluster */
                    step_radius = kLevyLocalMinRadiusPx + ((float)(esp_random() % 1000) * 0.001f) * (kLevyLocalMaxRadiusPx - kLevyLocalMinRadiusPx);
                }

                /* Directional vector on unit circle [0, 2pi) */
                float step_angle = ((float)(esp_random() % 6283) * 0.001f);
                float candX = s_startOffsetX + step_radius * cosf(step_angle);
                float candY = s_startOffsetY + step_radius * sinf(step_angle) + y_bias;

                /* Soft reflective boundary to keep target within natural ocular envelope */
                if (candX > 14.0f) candX = 14.0f - (candX - 14.0f);
                if (candX < -14.0f) candX = -14.0f + (-14.0f - candX);
                if (candY > 8.0f) candY = 8.0f - (candY - 8.0f);
                if (candY < -9.0f) candY = -9.0f + (-9.0f - candY);

                s_targetOffsetX = constrain(candX, -14.0f, 14.0f);
                s_targetOffsetY = constrain(candY, -9.0f, 8.0f);
            }

            float ds = sqrtf((s_targetOffsetX - s_startOffsetX) * (s_targetOffsetX - s_startOffsetX) +
                             (s_targetOffsetY - s_startOffsetY) * (s_targetOffsetY - s_startOffsetY));
            s_gazeDuration = compute_saccade_duration_ms(ds);
            float interval_scale = getPersonalityIdleIntervalScale();
            if (isCircadianSleepTime()) {
                interval_scale *= 2.2f; /* Drowsy, slow-spaced saccades at night */
            }
            s_nextGazeTime = now + s_gazeDuration + (uint32_t)(interval_scale * (float)(esp_random() % 2500 + 3200));
            s_gazeStartTime = now;
            s_inSaccade = true;
        }
    }

    if (s_inSaccade) {
        float elapsed = (float)(now - s_gazeStartTime);
        float progress = elapsed / (float)s_gazeDuration;

        if (progress >= 1.0f) {
            g_currentOffsetX = s_targetOffsetX;
            g_currentOffsetY = s_targetOffsetY;
            s_inSaccade = false;
            s_nextMicrosaccadeTime = now + kMicrosaccadeIntervalMinMs + (esp_random() % kMicrosaccadeIntervalRangeMs);
            s_inMicrosaccade = false;
        } else {
            float s = eval_minimum_jerk_spline(progress);
            float distX = s_targetOffsetX - s_startOffsetX;
            float distY = s_targetOffsetY - s_startOffsetY;
            g_currentOffsetX = s_startOffsetX + (distX * s);
            g_currentOffsetY = s_startOffsetY + (distY * s);
        }
    } else {
        if (s_inMicrosaccade) {
            float elapsed_micro = (float)(now - s_microsaccadeStartTime);
            float prog_micro = elapsed_micro / (float)kMicrosaccadeDurationMs;
            if (prog_micro >= 1.0f) {
                g_currentOffsetX = s_microsaccadeTargetX;
                g_currentOffsetY = s_microsaccadeTargetY;
                s_inMicrosaccade = false;
            } else {
                float sm = easeInOutCubic(prog_micro);
                g_currentOffsetX = s_microsaccadeStartX + (s_microsaccadeTargetX - s_microsaccadeStartX) * sm;
                g_currentOffsetY = s_microsaccadeStartY + (s_microsaccadeTargetY - s_microsaccadeStartY) * sm;
            }
        } else {
            /* Periodic anti-fading microsaccade trigger */
            if (s_nextMicrosaccadeTime == 0) {
                s_nextMicrosaccadeTime = now + kMicrosaccadeIntervalMinMs + (esp_random() % kMicrosaccadeIntervalRangeMs);
            }
            float errX = g_currentOffsetX - s_targetOffsetX;
            float errY = g_currentOffsetY - s_targetOffsetY;
            float distErr = sqrtf(errX * errX + errY * errY);

            if (now >= s_nextMicrosaccadeTime && distErr >= kMicrosaccadeThresholdPx) {
                s_inMicrosaccade = true;
                s_microsaccadeStartTime = now;
                s_microsaccadeStartX = g_currentOffsetX;
                s_microsaccadeStartY = g_currentOffsetY;
                /* Recenter to target anchor with sub-pixel landing jitter */
                s_microsaccadeTargetX = s_targetOffsetX + ((float)(esp_random() % 100) - 50.0f) * 0.001f;
                s_microsaccadeTargetY = s_targetOffsetY + ((float)(esp_random() % 100) - 50.0f) * 0.001f;
                s_nextMicrosaccadeTime = now + kMicrosaccadeIntervalMinMs + (esp_random() % kMicrosaccadeIntervalRangeMs);
            } else {
                /* Ornstein-Uhlenbeck stochastic drift with mean reversion toward fixation target anchor */
                float u1 = ((float)(esp_random() % 2000) - 1000.0f) * 0.001f;
                float u2 = ((float)(esp_random() % 2000) - 1000.0f) * 0.001f;
                float sq_dt = sqrtf(dt);

                float d_drift_x = -kOUMeanReversionRate * errX * dt + kOUVolatilitySigma * sq_dt * u1;
                float d_drift_y = -kOUMeanReversionRate * errY * dt + kOUVolatilitySigma * sq_dt * u2;

                g_currentOffsetX += d_drift_x;
                g_currentOffsetY += d_drift_y;
            }
        }
    }

    g_currentOffsetX = constrain(g_currentOffsetX, -17.5f, 17.5f);
    g_currentOffsetY = constrain(g_currentOffsetY, -12.0f, 11.0f);
}

float getAffectiveEyeScaleX(void) {
    return g_currentEyeScaleX;
}

float getAffectiveEyeScaleY(void) {
    return g_currentEyeScaleY;
}

float getOcularVergence(void) {
    return g_currentVergence;
}

/* Lid-Saccade Synkinesis Parameters (von Graefe's following law) */
static const float kLidSynkinesisMaxGazeY = 12.0f;
static const float kLidSynkinesisUpGain = 0.06f;    /* Palpebral widening during upward gaze */
static const float kLidSynkinesisDownGain = 0.09f;  /* Palpebral narrowing following downward gaze */
static const float kMinPalpebralSynkinesis = 0.05f; /* Preserve complete eyelid closure during blinks */
static const float kMaxPalpebralSynkinesis = 1.08f;
static const float kLidFissureTrackingGain = 0.15f; /* Fissure slit vertical following factor */

float getLidSaccadeSynkinesisAperture(float currentAperture, float gazeOffsetY) {
    if (currentAperture <= kMinPalpebralSynkinesis) {
        return 0.0f; /* Preserve complete eyelid closure during blinks and deep sleep */
    }

    float norm_y = gazeOffsetY / kLidSynkinesisMaxGazeY;
    norm_y = constrain(norm_y, -1.0f, 1.0f);

    /* Upward gaze (norm_y < 0) elevates upper eyelid; downward gaze narrows aperture */
    float delta = (norm_y < 0.0f) ? (-kLidSynkinesisUpGain * norm_y) : (-kLidSynkinesisDownGain * norm_y);
    float synkinesis_aperture = currentAperture + delta;

    return constrain(synkinesis_aperture, kMinPalpebralSynkinesis, kMaxPalpebralSynkinesis);
}

float getLidSaccadeFissureOffsetY(float gazeOffsetY) {
    return kLidFissureTrackingGain * gazeOffsetY;
}

/* Palpebral parameters and time constants for biological sleep-struggle dynamics */
static const float kDrowsyThreshold = 0.25f;
static const float kDroopTauBaseSec = 0.70f;
static const float kDroopTauScaleSec = 0.65f;
static const float kNodBasePx = 2.2f;
static const float kNodScalePx = 1.3f;
static const float kNodRelaxRate = 3.5f;
static const float kNodHoverRate = 4.0f;
static const float kNodReboundRate = 18.0f;
static const float kHoverBaseDurationSec = 0.35f;
static const float kHoverWillScaleSec = 1.10f;
static const float kHoverNoiseScaleSec = 0.25f;
static const float kHoverMicroTremorAmp = 0.018f;
static const float kHoverMicroTremorFreq = 3.5f;
static const float kSnapTauBaseSec = 0.11f;
static const float kSnapTauScaleSec = 0.05f;
static const float kEffortBaseDurationSec = 0.45f;
static const float kEffortWillScaleSec = 2.40f;
static const float kEffortSleepDamp = 0.55f;
static const float kMinPalpebralAperture = 0.05f;
static const float kMaxPalpebralAperture = 1.0f;
static const float kMaxNodOffsetPx = 4.0f;

typedef enum {
    DROWSY_PHASE_AWAKE = 0,
    DROWSY_PHASE_SINKING,
    DROWSY_PHASE_HOVERING,
    DROWSY_PHASE_RECOVERY_SNAP,
    DROWSY_PHASE_EFFORT_HOLD
} DrowsyStrugglePhase;

static DrowsyStrugglePhase s_drowsy_phase = DROWSY_PHASE_AWAKE;
static float s_drowsy_aperture = 1.0f;
static float s_drowsy_nod_y = 0.0f;
static float s_snap_target = 0.85f;
static float s_hover_duration = 0.80f;
static float s_effort_duration = 1.20f;
static float s_phase_timer = 0.0f;
static float s_hover_micro_phase = 0.0f;

static inline float get_kinematic_random_01(void) {
#if defined(ESP_PLATFORM) || defined(ARDUINO)
    return (float)(esp_random() % 10000) / 10000.0f;
#else
    return (float)(rand() % 10000) / 10000.0f;
#endif
}

void resetDrowsyEyelidState(void) {
    s_drowsy_phase = DROWSY_PHASE_AWAKE;
    s_drowsy_aperture = 1.0f;
    s_drowsy_nod_y = 0.0f;
    s_phase_timer = 0.0f;
}

bool isDrowsyStruggleActive(void) {
    return (s_drowsy_phase != DROWSY_PHASE_AWAKE);
}

float getDrowsyAperture(void) {
    return s_drowsy_aperture;
}

float getDrowsyNodOffsetY(void) {
    return s_drowsy_nod_y;
}

void updateDrowsyEyelidKinematics(float dt_sec, float sleep_pressure, float volitional_will, float droop_target) {
    if (dt_sec <= 0.0001f) dt_sec = 0.016666f;
    if (dt_sec > 0.10f) dt_sec = 0.10f;

    if (sleep_pressure < kDrowsyThreshold) {
        if (s_drowsy_phase != DROWSY_PHASE_AWAKE) {
            float alpha = 1.0f - expf(-8.0f * dt_sec);
            s_drowsy_aperture += (1.0f - s_drowsy_aperture) * alpha;
            s_drowsy_nod_y += (0.0f - s_drowsy_nod_y) * alpha;
            if (s_drowsy_aperture > 0.98f && fabsf(s_drowsy_nod_y) < 0.1f) {
                s_drowsy_aperture = 1.0f;
                s_drowsy_nod_y = 0.0f;
                s_drowsy_phase = DROWSY_PHASE_AWAKE;
            }
        }
        return;
    }

    if (s_drowsy_phase == DROWSY_PHASE_AWAKE) {
        s_drowsy_phase = DROWSY_PHASE_SINKING;
        s_phase_timer = 0.0f;
    }

    s_phase_timer += dt_sec;

    switch (s_drowsy_phase) {
        case DROWSY_PHASE_SINKING: {
            float tau_droop = kDroopTauBaseSec + kDroopTauScaleSec * sleep_pressure;
            float alpha_droop = 1.0f - expf(-dt_sec / tau_droop);
            s_drowsy_aperture += (droop_target - s_drowsy_aperture) * alpha_droop;

            float target_nod = (1.0f - s_drowsy_aperture) * (kNodBasePx + kNodScalePx * sleep_pressure);
            float alpha_nod = 1.0f - expf(-kNodRelaxRate * dt_sec);
            s_drowsy_nod_y += (target_nod - s_drowsy_nod_y) * alpha_nod;

            if (fabsf(s_drowsy_aperture - droop_target) < 0.035f || s_phase_timer > 3.0f) {
                s_drowsy_phase = DROWSY_PHASE_HOVERING;
                s_phase_timer = 0.0f;
                s_hover_duration = kHoverBaseDurationSec + kHoverWillScaleSec * (1.0f - volitional_will)
                                 + kHoverNoiseScaleSec * get_kinematic_random_01();
            }
            break;
        }

        case DROWSY_PHASE_HOVERING: {
            s_hover_micro_phase += dt_sec * kHoverMicroTremorFreq;
            float micro_osc = kHoverMicroTremorAmp * sinf(s_hover_micro_phase);
            s_drowsy_aperture = droop_target + micro_osc;

            float target_nod = (1.0f - droop_target) * (kNodBasePx + kNodScalePx * sleep_pressure);
            float alpha_nod = 1.0f - expf(-kNodHoverRate * dt_sec);
            s_drowsy_nod_y += (target_nod - s_drowsy_nod_y) * alpha_nod;

            if (s_phase_timer >= s_hover_duration) {
                s_phase_timer = 0.0f;
                float p_fight = 0.35f + 0.60f * volitional_will - 0.20f * sleep_pressure;
                p_fight = constrain(p_fight, 0.15f, 0.95f);

                s_drowsy_phase = DROWSY_PHASE_RECOVERY_SNAP;
                if (get_kinematic_random_01() <= p_fight) {
                    s_snap_target = constrain(0.72f + 0.28f * volitional_will - 0.10f * sleep_pressure, 0.65f, 1.0f);
                } else {
                    s_snap_target = constrain(0.60f + 0.25f * volitional_will, 0.55f, 0.85f);
                }
            }
            break;
        }

        case DROWSY_PHASE_RECOVERY_SNAP: {
            float tau_snap = kSnapTauBaseSec + kSnapTauScaleSec * sleep_pressure;
            float alpha_snap = 1.0f - expf(-dt_sec / tau_snap);
            s_drowsy_aperture += (s_snap_target - s_drowsy_aperture) * alpha_snap;

            float alpha_head = 1.0f - expf(-kNodReboundRate * dt_sec);
            s_drowsy_nod_y += (0.0f - s_drowsy_nod_y) * alpha_head;

            if (s_drowsy_aperture >= s_snap_target - 0.03f || s_phase_timer > 0.45f) {
                s_drowsy_phase = DROWSY_PHASE_EFFORT_HOLD;
                s_phase_timer = 0.0f;
                s_effort_duration = kEffortBaseDurationSec
                                  + kEffortWillScaleSec * volitional_will * (1.0f - kEffortSleepDamp * sleep_pressure)
                                  + 0.30f * get_kinematic_random_01();
            }
            break;
        }

        case DROWSY_PHASE_EFFORT_HOLD: {
            s_drowsy_aperture = s_snap_target;
            float alpha_head = 1.0f - expf(-10.0f * dt_sec);
            s_drowsy_nod_y += (0.0f - s_drowsy_nod_y) * alpha_head;

            if (s_phase_timer >= s_effort_duration) {
                s_drowsy_phase = DROWSY_PHASE_SINKING;
                s_phase_timer = 0.0f;
            }
            break;
        }

        default:
            s_drowsy_phase = DROWSY_PHASE_AWAKE;
            break;
    }

    s_drowsy_aperture = constrain(s_drowsy_aperture, kMinPalpebralAperture, kMaxPalpebralAperture);
    s_drowsy_nod_y = constrain(s_drowsy_nod_y, 0.0f, kMaxNodOffsetPx);
}

void setAffectiveKinematicsProfile(Expression expr) {
    if ((int)expr >= 0 && (int)expr < NUM_EXPRESSIONS) {
        const AffectiveKinematicProfile* prof = &kAffectiveKinematicTable[expr];
        s_active_omega_mult = prof->omega_mult;
        s_active_zeta_mult = prof->zeta_mult;
        s_active_dur_mult = prof->duration_mult;
        s_active_glissade = prof->glissade_gain;
    }
}

AffectiveKinematicProfile getActiveAffectiveKinematicProfile(void) {
    AffectiveKinematicProfile prof;
    prof.omega_mult = s_active_omega_mult;
    prof.zeta_mult = s_active_zeta_mult;
    prof.duration_mult = s_active_dur_mult;
    prof.glissade_gain = s_active_glissade;
    return prof;
}
