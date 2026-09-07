/**
 * @file affective_engine.h
 * @brief 2D Russell Circumplex affective emotion dynamics and stochastic Langevin model.
 */

#ifndef LORE_AFFECTIVE_ENGINE_H
#define LORE_AFFECTIVE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Expression g_currentExpr;
extern float g_animFrame;
extern BlinkState g_blinkState;
extern float g_blinkEyeHeight;
extern uint32_t g_nextBlinkTime;

void updateBiologicalMoodEngine(void);
void setNextExpression(Expression newExpr);
void setManualExpression(int expr_code);
int getManualExpression(void);
bool isManualExpressionActive(void);
const char* getExpressionName(Expression expr);
float getEmotionValence(void);
float getEmotionArousal(void);

#ifdef __cplusplus
}
#endif

#endif /* LORE_AFFECTIVE_ENGINE_H */
