/**
 * @file autonomic_engine.h
 * @brief Autonomous biological brainstem dynamics, Matsuoka Central Pattern Generator (CPG),
 *        metabolic homeostasis, and continuous ocular soma synthesis for LoRe.
 */

#ifndef LORE_AUTONOMIC_ENGINE_H
#define LORE_AUTONOMIC_ENGINE_H

#include "src/types/lore_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float respiration_phase;    /* Continuous respiratory rhythm [-1.0, 1.0] */
    float metabolic_energy;     /* Internal homeostatic energy [0.0, 1.0] */
    float motor_tension;        /* Accumulated ocular strain / posture fatigue [0.0, 1.0] */
    float spontaneous_drive;    /* Volitional drive for spontaneous exploration [0.0, 1.0] */
    float nystagmus_x;          /* Physiological tremor X [-1.0, 1.0] px */
    float nystagmus_y;          /* Physiological tremor Y [-1.0, 1.0] px */
} AutonomicTelemetry;

/**
 * @brief Initialize the autonomic biological dynamics engine.
 */
void initAutonomicEngine(void);

/**
 * @brief Update the Matsuoka CPG, homeostatic metabolism, and continuous soma state.
 * @param dt_sec Delta time in seconds (typically ~0.0166s for 60 FPS).
 */
void updateAutonomicEngine(float dt_sec);

/**
 * @brief Retrieve the current physical ocular soma state for rendering.
 * @return Current OcularSomaState struct.
 */
OcularSomaState getOcularSomaState(void);

/**
 * @brief Retrieve internal autonomic telemetry (respiration, energy, tension).
 */
AutonomicTelemetry getAutonomicTelemetry(void);

/**
 * @brief Set external stimulus / perturbation into the autonomic nervous system.
 * @param stimulus_energy Intensity of external or internal perturbation [0.0, 1.0].
 */
void stimulateAutonomicEngine(float stimulus_energy);

/**
 * @brief Check if autonomic system has triggered a spontaneous blink / sigh.
 */
bool consumeSpontaneousBlinkTrigger(void);

#ifdef __cplusplus
}
#endif

#endif /* LORE_AUTONOMIC_ENGINE_H */
