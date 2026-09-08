/**
 * @file personality_engine.cpp
 * @brief Persistent personality trait system and circadian rhythm engine implementation.
 */

#include "src/ai/personality_engine.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
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
static bool s_is_sleep_time = false;
static bool s_is_deep_sleep_time = false;
static bool s_is_wakeup_time = false;
static float s_drowsiness = 0.0f;
static uint8_t s_effective_sunrise_hour = DEFAULT_SUNRISE_HOUR;
static uint8_t s_effective_sunrise_min = DEFAULT_SUNRISE_MIN;
static uint8_t s_effective_sunset_hour = DEFAULT_SUNSET_HOUR;
static uint8_t s_effective_sunset_min = DEFAULT_SUNSET_MIN;

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
    /* Synchronize sun times from weather observation under mutex */
    portENTER_CRITICAL(&g_weather_mutex);
    if (g_weather_info.sun_times_valid) {
        s_effective_sunrise_hour = g_weather_info.sunrise_hour;
        s_effective_sunrise_min = g_weather_info.sunrise_min;
        s_effective_sunset_hour = g_weather_info.sunset_hour;
        s_effective_sunset_min = g_weather_info.sunset_min;
    } else {
        s_effective_sunrise_hour = DEFAULT_SUNRISE_HOUR;
        s_effective_sunrise_min = DEFAULT_SUNRISE_MIN;
        s_effective_sunset_hour = DEFAULT_SUNSET_HOUR;
        s_effective_sunset_min = DEFAULT_SUNSET_MIN;
    }
    portEXIT_CRITICAL(&g_weather_mutex);

    time_t now_sec;
    time(&now_sec);
    bool time_synced = (now_sec > 1700000000);
    float phase_raw = 0.0f;

    if (time_synced) {
        struct tm timeinfo;
        localtime_r(&now_sec, &timeinfo);
        float hour_float = (float)timeinfo.tm_hour + (float)timeinfo.tm_min / 60.0f + (float)timeinfo.tm_sec / 3600.0f;

        /* 24-hour cycle: 0.0 at midnight 00:00, 0.5 at 12:00 noon */
        phase_raw = hour_float / 24.0f;
        s_circadian.phase_pct = phase_raw * 100.0f;

        int now_minutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        int sunrise_minutes = (int)s_effective_sunrise_hour * 60 + (int)s_effective_sunrise_min;
        int sunset_minutes = (int)s_effective_sunset_hour * 60 + (int)s_effective_sunset_min;

        /* Deep sleep interval: 01:00 AM to 05:30 AM (or until sunrise if earlier) */
        int deep_sleep_start = OLED_DEEP_SLEEP_START_HOUR * 60 + OLED_DEEP_SLEEP_START_MIN;
        int deep_sleep_end = OLED_DEEP_SLEEP_END_HOUR * 60 + OLED_DEEP_SLEEP_END_MIN;
        if (sunrise_minutes < deep_sleep_end) {
            deep_sleep_end = sunrise_minutes;
        }
        s_is_deep_sleep_time = (now_minutes >= deep_sleep_start && now_minutes < deep_sleep_end);

        /* Sleep state: 23:00 to sunrise */
        s_is_sleep_time = (now_minutes >= 23 * 60 || now_minutes < sunrise_minutes);

        /* Wakeup transition: from sunrise until sunrise + 75 minutes */
        s_is_wakeup_time = (now_minutes >= sunrise_minutes && now_minutes < sunrise_minutes + 75);

        /* Compute natural drowsiness factor [0.0, 1.0] */
        if (now_minutes >= 23 * 60) {
            /* Late night ramp: 23:00 to 24:00 */
            s_drowsiness = 0.70f + 0.25f * ((float)timeinfo.tm_min / 60.0f);
        } else if (now_minutes < sunrise_minutes) {
            /* Deep night peaceful sleep */
            s_drowsiness = 0.95f;
        } else if (s_is_wakeup_time) {
            /* Morning awakening: drowsiness decays smoothly from 0.55 down to 0.0 */
            float wake_progress = (float)(now_minutes - sunrise_minutes) / 75.0f;
            s_drowsiness = 0.55f * (1.0f - wake_progress);
        } else if (now_minutes >= sunset_minutes - 30) {
            /* Evening dusk wind-down: starts 30 min before sunset, rises towards bedtime */
            int evening_span = (23 * 60) - (sunset_minutes - 30);
            if (evening_span < 60) evening_span = 60;
            float evening_progress = (float)(now_minutes - (sunset_minutes - 30)) / (float)evening_span;
            s_drowsiness = 0.65f * evening_progress;
        } else {
            s_drowsiness = 0.0f;
        }

        /* Diurnal energy curve: Alertness peaks at 14:00 (phase = 0.583), troughs at 03:00 (phase = 0.125) */
        float theta = 6.2831853f * (phase_raw - 0.583f);
        float base_energy = 0.60f + 0.40f * cosf(theta);
        if (s_is_sleep_time) {
            base_energy = fminf(base_energy, 0.25f);
        }
        s_circadian.energy_level = constrain(base_energy, 0.20f, 1.0f);

        float mood_theta = 6.2831853f * (phase_raw - 0.50f);
        s_circadian.mood_baseline = constrain(0.025f + 0.18f * cosf(mood_theta), -0.20f, 0.25f);

        float act_theta = 6.2831853f * (phase_raw - 0.54f);
        float base_act = 0.55f + 0.45f * cosf(act_theta);
        if (s_is_sleep_time) {
            base_act = fminf(base_act, 0.15f);
        }
        s_circadian.activity_drive = constrain(base_act, 0.10f, 1.0f);
    } else {
        /* Fallback to relative elapsed time if NTP is not yet synchronized */
        uint32_t elapsed = millis() - s_circadian_epoch_ms;
        phase_raw = fmodf((float)elapsed, (float)CIRCADIAN_CYCLE_PERIOD_MS) / (float)CIRCADIAN_CYCLE_PERIOD_MS;
        s_circadian.phase_pct = phase_raw * 100.0f;

        float theta = 6.2831853f * (phase_raw - 0.375f);
        s_circadian.energy_level = constrain(0.65f + 0.35f * cosf(theta), 0.20f, 1.0f);

        float mood_theta = 6.2831853f * (phase_raw - 0.30f);
        s_circadian.mood_baseline = constrain(0.025f + 0.20f * cosf(mood_theta), -0.20f, 0.25f);

        float act_theta = 6.2831853f * (phase_raw - 0.30f);
        s_circadian.activity_drive = constrain(0.55f + 0.40f * cosf(act_theta), 0.10f, 1.0f);

        s_is_sleep_time = false;
        s_is_deep_sleep_time = false;
        s_is_wakeup_time = false;
        s_drowsiness = 0.0f;
    }
}

bool isCircadianSleepTime(void) {
    return s_is_sleep_time;
}

bool isCircadianDeepSleepTime(void) {
    return s_is_deep_sleep_time;
}

bool isCircadianWakeupTime(void) {
    return s_is_wakeup_time;
}

float getCircadianDrowsiness(void) {
    return s_drowsiness;
}

uint8_t getEffectiveSunriseHour(void) {
    return s_effective_sunrise_hour;
}

uint8_t getEffectiveSunriseMin(void) {
    return s_effective_sunrise_min;
}

uint8_t getEffectiveSunsetHour(void) {
    return s_effective_sunset_hour;
}

uint8_t getEffectiveSunsetMin(void) {
    return s_effective_sunset_min;
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
