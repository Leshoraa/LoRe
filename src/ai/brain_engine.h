/**
 * @file brain_engine.h
 * @brief On-Device TinyML / Micro-Brain Behavioral Neural Engine declarations for LoRe.
 */

#ifndef LORE_BRAIN_ENGINE_H
#define LORE_BRAIN_ENGINE_H

#include "src/types/lore_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float curiosity;    /* [0.0, 1.0]: Rises with novelty; seeks stimulation */
    float social;       /* [0.0, 1.0]: Rises with companionship; decays into loneliness when alone */
    float boredom;      /* [0.0, 1.0]: Rises during monotony; triggers ambient glance */
    float fatigue;      /* [0.0, 1.0]: Rises with hyperactivity; prevents overstimulation */
    float mischief;     /* [0.0, 1.0]: Playfulness trait; triggers teasing smirks */
} HomeostaticDrives;

typedef struct {
    float valence;              /* Affective valence [-1.0, +1.0] */
    float arousal;              /* Affective arousal [0.0, 1.0] */
    HomeostaticDrives drives;   /* 5 Homeostatic biological drives */
    float decision_logits[NUM_EXPRESSIONS];   /* Neural decision logits for each expression */
    float decision_probs[NUM_EXPRESSIONS];    /* Softmax probabilities for each expression */
    Expression dominant_expr;   /* Neural policy top chosen expression */
    char thought_summary[48];   /* Human-readable inner monologue */
    uint32_t interaction_sec;   /* Current session interaction seconds */
    uint32_t solitude_sec;      /* Current session solitude seconds */
    float bonding_level;        /* [0.0, 1.0]: Cumulative bonding level in NVS */
    uint32_t lifetime_sec;      /* Total cumulative lifetime interaction seconds */
    uint8_t memory_count;       /* Total active episodic memory entries */
    float memory_resonance;     /* [0.0, 1.0]: Latest top cosine similarity score */
    Expression last_recalled_expr; /* Expression of top resonant memory */
} BrainTelemetry;

void initBrainEngine(void);
void updateBrainEngine(float dt_sec);
BrainTelemetry getBrainTelemetry(void);
Expression sampleBrainExpressionPolicy(void);
const char* getBrainThoughtSummary(void);
void saveBrainMemoryNVS(void);
void loadBrainMemoryNVS(void);
float getBrainBondingLevel(void);
uint32_t getBrainLifetimeSec(void);

/* Borbély Two-Process Biological Sleep Model */
float getBiologicalSleepPressure(void);
bool sampleMicroSleepDecision(void);
float getBiologicalDozeDurationMs(void);

#ifdef __cplusplus
}
#endif

#endif /* LORE_BRAIN_ENGINE_H */
