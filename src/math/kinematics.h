/**
 * @file kinematics.h
 * @brief Biomechanical ocular kinematics, mass-spring-damper, and minimum-jerk solver.
 */

#ifndef LORE_KINEMATICS_H
#define LORE_KINEMATICS_H

#include <stdint.h>
#include <stdbool.h>
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern float g_currentOffsetX;
extern float g_currentOffsetY;
extern float g_currentVergence;
extern float g_currentEyeScale;
extern float g_currentEyeScaleX;
extern float g_currentEyeScaleY;
extern bool g_is_transitioning;

float eval_minimum_jerk_spline(float p);
uint32_t compute_saccade_duration_ms(float displacement_px);
float easeInOutCubic(float t);
float eval_elastic_bounce_ease(float t);
void compute_squash_stretch_factors(float progress, float arousal, float* scaleX, float* scaleY);
float blinkCloseEase(float t);
float blinkOpenEase(float t);
float customLerp(float a, float b, float t);
int getFilteredOx(float rawOffsetX);
int getFilteredOy(float rawOffsetY);
void resetHysteresisFilter(void);
void updateGazeSystem(void);
float getAffectiveEyeScaleX(void);
float getAffectiveEyeScaleY(void);
float getOcularVergence(void);

/* Biomechanical Drowsy Palpebral Dynamics & Sleep-Struggle Physical Law */
void updateDrowsyEyelidKinematics(float dt_sec, float sleep_pressure, float volitional_will, float droop_target);
float getDrowsyAperture(void);
float getDrowsyNodOffsetY(void);
bool isDrowsyStruggleActive(void);
void resetDrowsyEyelidState(void);

/* Biomechanical Ocular Naturalization: Lid-Saccade Synkinesis & Listing's Law */
float getLidSaccadeSynkinesisAperture(float currentAperture, float gazeOffsetY);
float getLidSaccadeFissureOffsetY(float gazeOffsetY);

static inline float getListingTorsionAngleRad(float gazeOffsetX, float gazeOffsetY) {
    const float kListingMaxOffsetX = 17.5f;
    const float kListingMaxOffsetY = 12.0f;
    const float kListingTorsionGainRad = 0.045f; /* ~2.6 degrees max biomechanical tilt */

    float norm_x = gazeOffsetX / kListingMaxOffsetX;
    float norm_y = gazeOffsetY / kListingMaxOffsetY;
    if (norm_x < -1.0f) norm_x = -1.0f;
    if (norm_x > 1.0f) norm_x = 1.0f;
    return kListingTorsionGainRad * (norm_x * norm_y);
}

/* Affective Saccade Kinematics & Neuromuscular Tone Modulation */
typedef struct AffectiveKinematicProfile {
    float omega_mult;     /* Multiplier for natural frequency (stiffness / response speed) */
    float zeta_mult;      /* Multiplier for damping ratio (springiness vs rigid stop) */
    float duration_mult;  /* Multiplier for saccade duration */
    float glissade_gain;  /* Glissade landing rebound amplitude */
} AffectiveKinematicProfile;

void setAffectiveKinematicsProfile(Expression expr);
AffectiveKinematicProfile getActiveAffectiveKinematicProfile(void);

#ifdef __cplusplus
}
#endif

#endif /* LORE_KINEMATICS_H */
