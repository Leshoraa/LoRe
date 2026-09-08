/**
 * @file personality_engine.h
 * @brief Persistent personality trait system and circadian rhythm engine declarations for LoRe.
 */

#ifndef LORE_PERSONALITY_ENGINE_H
#define LORE_PERSONALITY_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float boldness;     /* [0.0, 1.0]: Shy/avoidant gaze (0) to direct/confident gaze holder (1) */
    float volatility;   /* [0.0, 1.0]: Emotionally stable (0) to moody/reactive (1) */
    float playfulness;  /* [0.0, 1.0]: Serious/stoic (0) to mischievous/teasing (1) */
    float attachment;   /* [0.0, 1.0]: Independent/aloof (0) to clingy/social (1) */
} PersonalityTraits;

typedef struct {
    float energy_level;     /* [0.2, 1.0]: Overall alertness/responsiveness scaling factor */
    float mood_baseline;    /* [-0.20, +0.25]: Additive valence offset applied to Langevin target */
    float activity_drive;   /* [0.1, 1.0]: Desire for engagement, scales saccade frequency */
    float phase_pct;        /* [0.0, 100.0]: Current position in the cycle for telemetry */
} CircadianState;

void initPersonalityEngine(void);
void savePersonalityNVS(void);
void updateCircadianCycle(void);

bool isCircadianSleepTime(void);
bool isCircadianDeepSleepTime(void);
bool isCircadianWakeupTime(void);
float getCircadianDrowsiness(void);

uint8_t getEffectiveSunriseHour(void);
uint8_t getEffectiveSunriseMin(void);
uint8_t getEffectiveSunsetHour(void);
uint8_t getEffectiveSunsetMin(void);

PersonalityTraits getPersonalityTraits(void);
CircadianState getCircadianState(void);

float getPersonalityValenceSigma(void);
float getPersonalityArousalSigma(void);
float getPersonalityGazeOmega(void);
float getPersonalityGazeDamping(void);
float getPersonalityIdleGazeYBias(void);
float getPersonalityIdleIntervalScale(void);
uint32_t getPersonalityBlinkInterval(void);
uint32_t getPersonalityDoubleBlinkChance(void);
float getPersonalitySocialGain(void);
float getPersonalityMischiefGain(void);
float getPersonalityBoredomRate(void);
float getPersonalityAmbientGlanceScale(void);

#ifdef __cplusplus
}
#endif

#endif /* LORE_PERSONALITY_ENGINE_H */
