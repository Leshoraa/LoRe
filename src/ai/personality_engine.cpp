/**
 * @file personality_engine.cpp
 * @brief Persistent personality trait system and circadian rhythm engine implementation.
 */

#include "src/ai/personality_engine.h"
#include "src/config/lore_config.h"
#include <math.h>
#include <Arduino.h>
#include <esp_random.h>
#ifdef ARDUINO
#include <Preferences.h>
#endif

static PersonalityTraits s_traits = {
    .boldness    = PERSONALITY_DEFAULT_BOLDNESS,
    .volatility  = PERSONALITY_DEFAULT_VOLATILITY,
    .playfulness = PERSONALITY_DEFAULT_PLAYFULNESS,
    .attachment  = PERSONALITY_DEFAULT_ATTACHMENT
};

static CircadianState s_circadian = {
    .energy_level  = 0.60f,
    .mood_baseline = 0.0f,
    .activity_drive = 0.50f,
    .phase_pct     = 0.0f
};

static uint32_t s_circadian_epoch_ms = 0;

void initPersonalityEngine(void) {
#ifdef ARDUINO
    Preferences prefs;
    if (prefs.begin("lore_persona", true)) {
        s_traits.boldness    = prefs.getFloat("bold", PERSONALITY_DEFAULT_BOLDNESS);
        s_traits.volatility  = prefs.getFloat("volat", PERSONALITY_DEFAULT_VOLATILITY);
        s_traits.playfulness = prefs.getFloat("play", PERSONALITY_DEFAULT_PLAYFULNESS);
        s_traits.attachment  = prefs.getFloat("attach", PERSONALITY_DEFAULT_ATTACHMENT);
        prefs.end();

        s_traits.boldness    = constrain(s_traits.boldness, 0.0f, 1.0f);
        s_traits.volatility  = constrain(s_traits.volatility, 0.0f, 1.0f);
        s_traits.playfulness = constrain(s_traits.playfulness, 0.0f, 1.0f);
        s_traits.attachment  = constrain(s_traits.attachment, 0.0f, 1.0f);
    }
#endif
    s_circadian_epoch_ms = millis();
}

void savePersonalityNVS(void) {
#ifdef ARDUINO
    Preferences prefs;
    if (prefs.begin("lore_persona", false)) {
        prefs.putFloat("bold", s_traits.boldness);
        prefs.putFloat("volat", s_traits.volatility);
        prefs.putFloat("play", s_traits.playfulness);
        prefs.putFloat("attach", s_traits.attachment);
        prefs.end();
    }
#endif
}

void updateCircadianCycle(void) {
    uint32_t elapsed = millis() - s_circadian_epoch_ms;
    float phase_raw = fmodf((float)elapsed, (float)CIRCADIAN_CYCLE_PERIOD_MS) / (float)CIRCADIAN_CYCLE_PERIOD_MS;
    s_circadian.phase_pct = phase_raw * 100.0f;

    /* Alertness peaks mid-morning at 37.5% of cycle */
    float theta = 6.2831853f * (phase_raw - 0.375f);
    s_circadian.energy_level = constrain(0.65f + 0.35f * cosf(theta), 0.20f, 1.0f);

    float mood_theta = 6.2831853f * (phase_raw - 0.30f);
    s_circadian.mood_baseline = constrain(0.025f + 0.20f * cosf(mood_theta), -0.20f, 0.25f);

    float act_theta = 6.2831853f * (phase_raw - 0.30f);
    s_circadian.activity_drive = constrain(0.55f + 0.40f * cosf(act_theta), 0.10f, 1.0f);
}

PersonalityTraits getPersonalityTraits(void) {
    return s_traits;
}

CircadianState getCircadianState(void) {
    return s_circadian;
}

float getPersonalityValenceSigma(void) {
    float base = 0.012f + 0.020f * s_traits.volatility;
    return base * (0.70f + 0.30f * s_circadian.energy_level);
}

float getPersonalityArousalSigma(void) {
    float base = 0.009f + 0.015f * s_traits.volatility;
    return base * (0.70f + 0.30f * s_circadian.energy_level);
}

float getPersonalityGazeOmega(void) {
    float base = GAZE_NATURAL_FREQUENCY_RAD_S;
    float bold_factor = 0.85f + 0.30f * s_traits.boldness;
    float energy_factor = 0.80f + 0.20f * s_circadian.energy_level;
    return base * bold_factor * energy_factor;
}

float getPersonalityGazeDamping(void) {
    return 0.90f - 0.28f * s_traits.boldness;
}

float getPersonalityIdleGazeYBias(void) {
    float shyness = 1.0f - s_traits.boldness;
    float rest_factor = 1.0f + 0.5f * (1.0f - s_circadian.energy_level);
    return shyness * 2.5f * rest_factor;
}

float getPersonalityIdleIntervalScale(void) {
    float play_speed = 1.0f - 0.35f * s_traits.playfulness * s_circadian.activity_drive;
    float shy_slow = 1.0f + 0.40f * (1.0f - s_traits.boldness) * (1.0f - s_circadian.energy_level);
    return constrain(play_speed * shy_slow, 0.55f, 1.50f);
}

uint32_t getPersonalityBlinkInterval(void) {
    float base_min = 3500.0f;
    float base_range = 3500.0f;
    float volatility_speedup = 1.0f - 0.25f * s_traits.volatility;
    float energy_slowdown = 1.0f + 0.30f * (1.0f - s_circadian.energy_level);
    float total_scale = volatility_speedup * energy_slowdown;
    uint32_t random_offset = esp_random() % (uint32_t)(base_range * total_scale);
    return (uint32_t)(base_min * total_scale) + random_offset;
}

uint32_t getPersonalityDoubleBlinkChance(void) {
    float chance = 8.0f + 14.0f * s_traits.playfulness;
    return (uint32_t)chance;
}

float getPersonalitySocialGain(void) {
    return 0.15f + 0.20f * s_traits.attachment;
}

float getPersonalityMischiefGain(void) {
    return 0.08f + 0.14f * s_traits.playfulness;
}

float getPersonalityBoredomRate(void) {
    float play_factor = 1.0f + 0.40f * s_traits.playfulness;
    float energy_factor = 1.0f + 0.25f * (1.0f - s_circadian.energy_level);
    return 0.12f * play_factor * energy_factor;
}

float getPersonalityAmbientGlanceScale(void) {
    float curiosity_factor = 1.0f - 0.30f * s_traits.playfulness;
    float activity_factor = 1.0f - 0.20f * s_circadian.activity_drive;
    return constrain(curiosity_factor * activity_factor, 0.40f, 1.20f);
}
