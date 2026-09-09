/**
 * @file facial_renderer.h
 * @brief Rigid 2D facial rig composition, eye/mouth geometry, and expression transition easing for LoRe.
 */

#ifndef LORE_FACIAL_RENDERER_H
#define LORE_FACIAL_RENDERER_H

#include "src/types/lore_types.h"
#include <LovyanGFX.hpp>

void set_facial_canvas(LGFX_Sprite* p_canvas);
void drawAutonomousSoma(const OcularSomaState& soma, float offsetX, float offsetY, float palpebralAperture = 1.0f, float vergence = 0.0f);
void drawFace(Expression expr, float eyeHeightFactor, float offsetX, float offsetY, float frame = 0.0f, float vergence = 0.0f, float scale = 1.0f);
void drawMiniFace(Expression expr, float eyeHeightFactor, float offsetX, float offsetY, float scale = 0.5f);
void transitionExpression(Expression fromExpr, Expression toExpr, float durationMs = 170.0f);
void clearOcularParticles(void);

#endif /* LORE_FACIAL_RENDERER_H */
