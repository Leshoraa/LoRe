/**
 * @file gaze_engine.h
 * @brief Autonomous gaze coordination, stimulus integration, and Core 0 power management for LoRe.
 */

#ifndef LORE_GAZE_ENGINE_H
#define LORE_GAZE_ENGINE_H

#include "src/config/lore_config.h"
#include "src/types/lore_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes cross-core synchronization primitives and target state.
 */
void initGazeEngine(void);

/**
 * @brief Core 0 FreeRTOS background task handling stimulus decay, state timeouts, and DFS power scaling.
 */
void gazeTask(void *pvParameters);

/**
 * @brief Directs LoRe gaze toward virtual normalized coordinates via Web or BLE stimulus.
 * @param[in] normX Normalized horizontal coordinate in range [-1.0 (left), +1.0 (right)].
 * @param[in] normY Normalized vertical coordinate in range [-1.0 (up), +1.0 (down)].
 * @param[in] duration_ms Attention hold duration in milliseconds.
 */
void setVirtualTarget(float normX, float normY, float duration_ms);

/**
 * @brief Returns ambient luminance estimate for display auto-brightness scaling.
 * @return Estimated luminance in range [0.0, 255.0].
 */
float getAmbientLuminance(void);

#ifdef __cplusplus
}
#endif

#endif /* LORE_GAZE_ENGINE_H */
