/**
 * @file brain_engine.cpp
 * @brief On-Device TinyML / Micro-Brain Behavioral Neural Engine implementation.
 */

#include "src/ai/brain_engine.h"
#include "src/ai/memory_engine.h"
#include "src/math/affective_engine.h"
#include "src/ai/personality_engine.h"
#include "src/ai/autonomic_engine.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include <esp_random.h>
#ifdef ARDUINO
#include <Preferences.h>
#endif

/* Persistent Bonding & Companionship State */
static float s_bonding_level = 0.05f;
static uint32_t s_lifetime_interaction_sec = 0;
static uint32_t s_last_nvs_save_ms = 0;
static uint32_t s_last_bonding_grow_ms = 0;

/* Internal Homeostatic State Variables */
static HomeostaticDrives s_drives = {
    .curiosity = 0.35f,
    .social    = 0.50f,
    .boredom   = 0.15f,
    .fatigue   = 0.10f,
    .mischief  = 0.40f
};

static float s_presence_ema = 0.0f;
static float s_motion_energy_ema = 0.0f;
static float s_proximity_smooth = 0.0f;
static uint32_t s_interaction_start_ms = 0;
static uint32_t s_solitude_start_ms = 0;
static uint32_t s_total_interaction_sec = 0;
static uint32_t s_total_solitude_sec = 0;
static uint32_t s_last_update_ms = 0;

static float s_decision_logits[NUM_EXPRESSIONS] = {0};
static float s_decision_probs[NUM_EXPRESSIONS] = {0};
static Expression s_dominant_expr = EXPR_IDLE;
static char s_thought_summary[48] = "Booting cognitive engine...";

/* 12x8 Neural Weight Matrix: Maps [V, A, Cur, Soc, Bor, Fat, Mis, Prox] -> 12 Expressions */
static const float s_neural_weights[NUM_EXPRESSIONS][8] = {
    /* 0: IDLE - Dominant calm resting baseline */
    {  0.1f, -0.4f,  0.1f,  0.2f,  0.3f, -0.2f, -0.2f,  0.1f },
    /* 1: HAPPY - High Valence, High Social, High Proximity, High Bonding */
    {  1.8f,  0.5f,  0.3f,  1.4f, -0.8f, -0.4f,  0.4f,  0.6f },
    /* 2: ANGRY - Negative Valence, High Arousal, High Mischief, Low Social */
    { -1.8f,  1.5f, -0.2f, -1.0f,  0.2f,  0.1f,  1.2f, -0.3f },
    /* 3: SAD - Negative Valence, Low Arousal, Low Social, High Solitude */
    { -1.9f, -1.2f, -0.5f, -1.5f,  1.0f,  0.6f, -0.8f, -0.6f },
    /* 4: SURPRISED - High Arousal, High Curiosity, High Proximity Pulse */
    {  0.2f,  1.8f,  1.4f,  0.3f, -1.0f, -0.8f,  0.2f,  1.2f },
    /* 5: SUSPICIOUS - Negative Valence, Moderate Arousal, High Curiosity, Low Social */
    { -0.6f,  0.6f,  1.2f, -0.8f,  0.3f, -0.2f,  0.8f,  0.4f },
    /* 6: CURIOUS - Moderate Valence, Moderate Arousal, Very High Curiosity & Proximity */
    {  0.6f,  0.8f,  1.9f,  0.5f, -0.5f, -0.5f,  0.5f,  0.9f },
    /* 7: MISCHIEF - High Mischief Drive, High Arousal, Positive Valence */
    {  0.8f,  1.1f,  0.7f,  0.6f, -0.3f, -0.6f,  1.8f,  0.5f },
    /* 8: SLEEPY - High Fatigue, Low Arousal, Low Curiosity */
    { -0.2f, -1.8f, -0.9f, -0.4f,  0.5f,  2.0f, -0.7f, -0.5f },
    /* 9: COOL - Positive Valence, Moderate Social, Low Arousal, High Confidence */
    {  1.2f, -0.2f,  0.2f,  0.8f, -0.4f, -0.3f,  0.7f,  0.3f },
    /* 10: DIZZY - Very High Arousal, Elevated Fatigue, Rapid Disturbance */
    { -0.5f,  1.7f, -0.4f, -0.2f, -0.6f,  1.4f,  0.3f,  0.8f },
    /* 11: CRYING - Extreme Negative Valence, Low Social, High Distress */
    { -2.2f,  0.8f, -0.6f, -1.6f,  0.8f,  0.9f, -1.0f, -0.7f }
};

static const float s_neural_biases[NUM_EXPRESSIONS] = {
     1.20f,  /* 0: IDLE */
    -0.40f,  /* 1: HAPPY */
    -1.50f,  /* 2: ANGRY */
    -1.40f,  /* 3: SAD */
    -1.20f,  /* 4: SURPRISED */
    -1.30f,  /* 5: SUSPICIOUS */
    -0.80f,  /* 6: CURIOUS */
    -1.00f,  /* 7: MISCHIEF */
    -0.60f,  /* 8: SLEEPY */
    -1.10f,  /* 9: COOL */
    -1.60f,  /* 10: DIZZY */
    -1.80f   /* 11: CRYING */
};

void loadBrainMemoryNVS(void) {
#ifdef ARDUINO
    Preferences prefs;
    if (prefs.begin("lore_brain", true)) {
        s_bonding_level = prefs.getFloat("bonding", 0.05f);
        s_lifetime_interaction_sec = prefs.getULong("life_sec", 0);
        prefs.end();
        s_bonding_level = constrain(s_bonding_level, 0.0f, 1.0f);
    }
#endif
    loadMemoryNVS();
}

void saveBrainMemoryNVS(void) {
#ifdef ARDUINO
    Preferences prefs;
    if (prefs.begin("lore_brain", false)) {
        prefs.putFloat("bonding", s_bonding_level);
        prefs.putULong("life_sec", s_lifetime_interaction_sec);
        prefs.end();
        s_last_nvs_save_ms = millis();
    }
#endif
    saveMemoryNVS();
}

float getBrainBondingLevel(void) {
    return s_bonding_level;
}

uint32_t getBrainLifetimeSec(void) {
    return s_lifetime_interaction_sec;
}

void initBrainEngine(void) {
    initMemoryEngine();
    loadBrainMemoryNVS();

    s_drives.curiosity = 0.35f;
    s_drives.social    = 0.45f + 0.25f * s_bonding_level;
    s_drives.boredom   = 0.10f;
    s_drives.fatigue   = 0.05f;
    s_drives.mischief  = 0.40f;

    s_last_update_ms = millis();
    s_solitude_start_ms = s_last_update_ms;
    s_last_nvs_save_ms = s_last_update_ms;
    s_last_bonding_grow_ms = s_last_update_ms;
    snprintf(s_thought_summary, sizeof(s_thought_summary), "Observing surroundings...");
}

void updateBrainEngine(float dt_sec) {
    if (dt_sec <= 0.001f) dt_sec = 0.050f;
    dt_sec = constrain(dt_sec, 0.01f, 0.50f);

    unsigned long now = millis();

    /* 1. Sensory Integration */
    TrackTarget target;
    portENTER_CRITICAL(&g_target_mutex);
    target = g_current_target;
    portEXIT_CRITICAL(&g_target_mutex);

    float raw_pres = (target.detected && target.human_likelihood > HUMAN_LIKELIHOOD_THRESHOLD) ? 1.0f : 0.0f;
    s_presence_ema = s_presence_ema * 0.90f + raw_pres * 0.10f;
    bool is_detected = (s_presence_ema > 0.50f);

    float raw_motion = (target.detected) ? fminf(1.0f, (fabsf(target.vx) + fabsf(target.vy)) * 0.05f + target.total_energy * 0.02f) : 0.0f;
    s_motion_energy_ema = s_motion_energy_ema * 0.85f + raw_motion * 0.15f;

    if (target.detected) {
        s_proximity_smooth = s_proximity_smooth * 0.85f + target.proximity * 0.15f;
    } else {
        s_proximity_smooth *= 0.95f;
    }

    static bool s_was_detected = false;
    static uint32_t s_last_shock_mem_s = 0;
    static uint32_t s_last_joy_mem_s = 0;
    static uint32_t s_last_solitude_mem_s = 0;
    uint32_t now_s = now / 1000;

    if (is_detected) {
        if (s_interaction_start_ms == 0) s_interaction_start_ms = now;
        s_total_interaction_sec = (now - s_interaction_start_ms) / 1000;
        s_solitude_start_ms = 0;
        s_total_solitude_sec = 0;

        /* Advance cumulative lifetime companionship & bonding level */
        if (now - s_last_bonding_grow_ms >= 10000) {
            uint32_t delta_s = (now - s_last_bonding_grow_ms) / 1000;
            s_last_bonding_grow_ms = now;
            s_lifetime_interaction_sec += delta_s;
            s_bonding_level = fminf(1.0f, s_bonding_level + (float)delta_s * 0.000011f);
        }

        /* Salient Memory Ingestion: Happy bonding milestone */
        if (s_dominant_expr == EXPR_HAPPY && s_bonding_level > 0.30f && s_total_interaction_sec > 25) {
            if (now_s - s_last_joy_mem_s > 60) {
                s_last_joy_mem_s = now_s;
                float v_now = getEmotionValence();
                float a_now = getEmotionArousal();
                float mem_v[8] = { v_now, a_now, s_drives.curiosity, s_drives.social, s_drives.boredom, s_drives.fatigue, s_drives.mischief, s_proximity_smooth };
                recordEpisodicMemory(mem_v, EXPR_HAPPY, 0.75f + 0.20f * s_bonding_level, v_now);
            }
        }
    } else {
        /* Salient Memory Ingestion: End of sustained interaction session */
        if (s_was_detected && s_total_interaction_sec > 15) {
            float v_now = getEmotionValence();
            float a_now = getEmotionArousal();
            float mem_v[8] = { v_now, a_now, s_drives.curiosity, s_drives.social, s_drives.boredom, s_drives.fatigue, s_drives.mischief, s_proximity_smooth };
            float salience = fminf(1.0f, 0.40f + 0.30f * s_bonding_level + 0.01f * (float)s_total_interaction_sec);
            recordEpisodicMemory(mem_v, s_dominant_expr, salience, v_now);
        }

        if (s_solitude_start_ms == 0) s_solitude_start_ms = now;
        s_total_solitude_sec = (now - s_solitude_start_ms) / 1000;
        s_interaction_start_ms = 0;
        s_total_interaction_sec = 0;
        s_last_bonding_grow_ms = now;

        /* Salient Memory Ingestion: Deep solitude milestone */
        if ((s_total_solitude_sec == 180 || s_total_solitude_sec == 420) && (now_s - s_last_solitude_mem_s > 30)) {
            s_last_solitude_mem_s = now_s;
            float v_now = getEmotionValence();
            float a_now = getEmotionArousal();
            float mem_v[8] = { v_now, a_now, s_drives.curiosity, s_drives.social, s_drives.boredom, s_drives.fatigue, s_drives.mischief, s_proximity_smooth };
            Expression sol_expr = EXPR_IDLE;
            recordEpisodicMemory(mem_v, sol_expr, 0.60f, v_now);
        }
    }
    s_was_detected = is_detected;

    /* Periodic Flash Memory Wear-Leveling Save (Every 300 Seconds) */
    if (now - s_last_nvs_save_ms >= 300000) {
        saveBrainMemoryNVS();
        savePersonalityNVS();
    }

    /* 2. Homeostatic Biological Drive Evolution */
    float d_curiosity = (0.35f * s_motion_energy_ema * (1.0f - 0.4f * s_presence_ema) - 0.08f * s_drives.curiosity);
    s_drives.curiosity = constrain(s_drives.curiosity + d_curiosity * dt_sec, 0.0f, 1.0f);

    float social_gain = getPersonalitySocialGain();
    float d_social = (social_gain * s_presence_ema - 0.05f * (1.0f - s_presence_ema));
    s_drives.social = constrain(s_drives.social + d_social * dt_sec, 0.0f, 1.0f);

    float boredom_rate = getPersonalityBoredomRate();
    float d_boredom = (boredom_rate * (1.0f - s_motion_energy_ema) * (1.0f - 0.5f * s_presence_ema) - 0.20f * (s_motion_energy_ema + 0.3f * s_presence_ema));
    s_drives.boredom = constrain(s_drives.boredom + d_boredom * dt_sec, 0.0f, 1.0f);

    float d_fatigue = (0.30f * (s_motion_energy_ema * s_presence_ema) - 0.15f * (1.0f - s_motion_energy_ema));
    s_drives.fatigue = constrain(s_drives.fatigue + d_fatigue * dt_sec, 0.0f, 1.0f);

    float noise_m = ((float)(esp_random() % 1000) - 500.0f) * 0.0001f;
    float mischief_gain = getPersonalityMischiefGain();
    float d_mischief = (mischief_gain * (s_drives.social * (1.0f - s_drives.fatigue)) - 0.08f * s_drives.mischief + noise_m);
    s_drives.mischief = constrain(s_drives.mischief + d_mischief * dt_sec, 0.0f, 1.0f);

    /* Salient Memory Ingestion: Sudden High-Arousal event */
    float V = getEmotionValence();
    float A = getEmotionArousal();
    if (A > 0.82f && s_motion_energy_ema > 0.75f && (now_s - s_last_shock_mem_s > 20)) {
        s_last_shock_mem_s = now_s;
        float mem_v[8] = { V, A, s_drives.curiosity, s_drives.social, s_drives.boredom, s_drives.fatigue, s_drives.mischief, s_proximity_smooth };
        Expression spike_expr = EXPR_HAPPY;
        recordEpisodicMemory(mem_v, spike_expr, 0.85f, V);
    }

    /* 3. Neural Policy Feedforward Inference & Associative Episodic Recall */
    float state_vector[8] = {
        V,
        A,
        s_drives.curiosity,
        s_drives.social,
        s_drives.boredom,
        s_drives.fatigue,
        s_drives.mischief,
        s_proximity_smooth
    };

    EpisodicRecallResult recall = queryMemoryResonance(state_vector);

    float max_logit = -999.0f;
    for (int i = 0; i < NUM_EXPRESSIONS; i++) {
        float sum = s_neural_biases[i];
        for (int j = 0; j < 8; j++) {
            sum += s_neural_weights[i][j] * state_vector[j];
        }

        if (i == EXPR_HAPPY) {
            sum += 0.8f * s_bonding_level;
        } else if (i == EXPR_COOL) {
            sum += 0.3f * s_bonding_level;
        }

        sum += recall.memory_logits_delta[i];

        s_decision_logits[i] = sum;
        if (sum > max_logit) max_logit = sum;
    }

    float tau = 0.85f;
    float sum_exp = 0.0f;
    for (int i = 0; i < NUM_EXPRESSIONS; i++) {
        s_decision_probs[i] = expf((s_decision_logits[i] - max_logit) / tau);
        sum_exp += s_decision_probs[i];
    }
    float best_prob = -1.0f;
    for (int i = 0; i < NUM_EXPRESSIONS; i++) {
        s_decision_probs[i] /= sum_exp;
        if (s_decision_probs[i] > best_prob) {
            best_prob = s_decision_probs[i];
            s_dominant_expr = (Expression)i;
        }
    }

    /* 4. Update Cognitive Inner Monologue Summary */
    PersonalityTraits traits = getPersonalityTraits();
    CircadianState circa = getCircadianState();

    float sleep_p = getBiologicalSleepPressure();
    if (s_drives.fatigue > 0.70f || sleep_p > 0.60f) {
        float will = getVolitionalVigilance();
        if (is_detected && will > 0.30f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Fighting sleep... staying with you");
        else if (will > 0.40f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Heavy eyelids... trying to stay awake");
        else if (circa.energy_level < 0.40f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "So tired... everything is heavy");
        else if (traits.playfulness > 0.60f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Sleepy but still wanna play...");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Need to rest for a bit...");
    } else if (recall.resonance_score > 0.70f && recall.top_k_count > 0 && esp_random() % 100 < 45) {
        snprintf(s_thought_summary, sizeof(s_thought_summary), "%s", recall.recall_context);
    } else if (circa.energy_level < 0.35f && !is_detected) {
        if (traits.boldness < 0.40f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Getting drowsy... quiet here");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Low energy. Conserving...");
    } else if (s_bonding_level > 0.40f && is_detected && s_total_interaction_sec > 15) {
        if (traits.attachment > 0.60f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "So glad you are here! Stay close");
        else if (traits.playfulness > 0.60f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Hey companion! Wanna play?");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Comfortable with you nearby");
    } else if (traits.boldness < 0.35f && is_detected && s_total_interaction_sec < 5) {
        snprintf(s_thought_summary, sizeof(s_thought_summary), "Someone appeared... (cautious)");
    } else if (traits.boldness > 0.65f && is_detected && s_total_interaction_sec > 10) {
        snprintf(s_thought_summary, sizeof(s_thought_summary), "Watching you directly. Curious.");
    } else if (s_drives.mischief > 0.65f && s_presence_ema > 0.4f) {
        if (traits.playfulness > 0.70f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Hehe... feeling mischievous!");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Feeling cheeky right now");
    } else if (s_drives.curiosity > 0.60f) {
        if (circa.energy_level > 0.70f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Alert! Tracking something...");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Curious about that movement...");
    } else if (s_drives.boredom > 0.65f) {
        if (traits.playfulness > 0.60f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "So bored... need excitement!");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Nothing happening. Waiting...");
    } else if (s_total_solitude_sec > 120 && s_drives.social < 0.25f) {
        if (traits.attachment > 0.60f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Lonely... missing company");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Alone. That is fine for now.");
    } else if (circa.energy_level > 0.85f && !is_detected) {
        snprintf(s_thought_summary, sizeof(s_thought_summary), "Full energy! Ready for anything");
    } else if (is_detected) {
        if (s_total_interaction_sec > 30 && traits.boldness > 0.50f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Settled in. Watching calmly.");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Observing companion...");
    } else {
        if (circa.activity_drive > 0.70f)
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Looking around for stimulation");
        else
            snprintf(s_thought_summary, sizeof(s_thought_summary), "Observing quietly...");
    }
}

Expression sampleBrainExpressionPolicy(void) {
    float r = (float)(esp_random() % 10000) / 10000.0f;
    float cum = 0.0f;
    for (int i = 0; i < NUM_EXPRESSIONS; i++) {
        cum += s_decision_probs[i];
        if (r <= cum || i == NUM_EXPRESSIONS - 1) {
            return (Expression)i;
        }
    }
    return s_dominant_expr;
}

BrainTelemetry getBrainTelemetry(void) {
    BrainTelemetry t;
    t.valence = getEmotionValence();
    t.arousal = getEmotionArousal();
    t.drives  = s_drives;
    memcpy(t.decision_logits, s_decision_logits, sizeof(s_decision_logits));
    memcpy(t.decision_probs, s_decision_probs, sizeof(s_decision_probs));
    t.dominant_expr = s_dominant_expr;
    snprintf(t.thought_summary, sizeof(t.thought_summary), "%s", s_thought_summary);
    t.interaction_sec = s_total_interaction_sec;
    t.solitude_sec = s_total_solitude_sec;
    t.bonding_level = s_bonding_level;
    t.lifetime_sec = s_lifetime_interaction_sec;
    t.memory_count = getEpisodicMemoryCount();
    t.memory_resonance = getLatestMemoryResonance();
    t.last_recalled_expr = getLatestRecalledExpression();
    return t;
}

const char* getBrainThoughtSummary(void) {
    return s_thought_summary;
}

float getBiologicalSleepPressure(void) {
    float circadian_drowsiness = getCircadianDrowsiness();
    float arousal = getEmotionArousal();

    /* Borbély Two-Process Model:
     * Process C (Circadian clock drive) + Process S (Homeostatic sleep debt: fatigue + boredom)
     * dampened by sympathetic nervous system arousal */
    float raw_pressure = (0.55f * circadian_drowsiness)
                       + (0.45f * s_drives.fatigue)
                       + (0.25f * s_drives.boredom)
                       - (0.40f * arousal);

    return constrain(raw_pressure, 0.0f, 1.0f);
}

static const float kVigilanceCuriosityWeight = 0.30f;
static const float kVigilanceSocialWeight    = 0.25f;
static const float kVigilanceBondingWeight   = 0.20f;
static const float kVigilancePlayWeight      = 0.15f;
static const float kVigilanceMischiefWeight  = 0.10f;
static const float kVigilanceArousalWeight   = 0.25f;
static const float kVigilanceFatigueWeight   = 0.35f;
static const float kVigilanceBoredomWeight   = 0.25f;

static const float kDroopPressureWeight = 0.82f;
static const float kDroopEnergyWeight   = 0.18f;
static const float kDroopWillResistance = 0.42f;
static const float kMinDroopAperture    = 0.12f;
static const float kMaxDroopAperture    = 0.92f;

float getVolitionalVigilance(void) {
    PersonalityTraits traits = getPersonalityTraits();
    float arousal = getEmotionArousal();

    float social_drive = s_drives.social * (0.60f + 0.40f * s_presence_ema);
    float will = (kVigilanceCuriosityWeight * s_drives.curiosity)
               + (kVigilanceSocialWeight    * social_drive)
               + (kVigilanceBondingWeight   * s_bonding_level)
               + (kVigilancePlayWeight      * traits.playfulness)
               + (kVigilanceMischiefWeight  * s_drives.mischief)
               + (kVigilanceArousalWeight   * arousal)
               - (kVigilanceFatigueWeight   * s_drives.fatigue)
               - (kVigilanceBoredomWeight   * s_drives.boredom);

    return constrain(will, 0.0f, 1.0f);
}

float getBiologicalDroopAperture(void) {
    float pressure = getBiologicalSleepPressure();
    if (pressure < 0.25f) {
        return 1.0f;
    }

    AutonomicTelemetry auto_tel = getAutonomicTelemetry();
    float energy = auto_tel.metabolic_energy;
    float will = getVolitionalVigilance();

    float sag = (kDroopPressureWeight * pressure + kDroopEnergyWeight * (1.0f - energy))
              * (1.0f - kDroopWillResistance * will);
    float droop_aperture = 1.0f - sag;

    return constrain(droop_aperture, kMinDroopAperture, kMaxDroopAperture);
}

bool sampleMicroSleepDecision(void) {
    float pressure = getBiologicalSleepPressure();
    if (pressure < 0.25f) return false;

    float arousal = getEmotionArousal();
    /* Probability of falling into brief micro-sleep doze during eyelid closure */
    float p_doze = constrain(0.60f * pressure - 0.20f * arousal, 0.0f, 0.65f);
    float roll = (float)(esp_random() % 1000) / 1000.0f;
    return (roll < p_doze);
}

float getBiologicalDozeDurationMs(void) {
    float pressure = getBiologicalSleepPressure();
    /* Biological doze duration scales dynamically with sleep debt:
     * 800 ms (light microsleep) up to 2200 ms (heavy exhaustion doze) */
    return 800.0f + 1400.0f * pressure;
}

float getBrainCuriosityDrive(void) {
    /* Expose current homeostatic curiosity drive for exploratory ocular Lévy flight modulation */
    return s_drives.curiosity;
}

