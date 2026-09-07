/**
 * @file telemetry_service.cpp
 * @brief Telemetry snapshot serialization service implementation for LoRe.
 */

#include "src/net/telemetry_service.h"
#include "src/types/lore_types.h"
#include "src/config/lore_config.h"
#include "src/core/gaze_engine.h"
#include "src/core/display_engine.h"
#include "src/ai/brain_engine.h"
#include "src/ai/personality_engine.h"
#include "src/math/affective_engine.h"
#include <Arduino.h>
#include <stdio.h>
#include <esp_heap_caps.h>

void formatTelemetryJson(char* json, size_t max_len) {
    if (!json || max_len == 0) return;

    TrackTarget target;
    int num_cands = 0;
    int insp_idx = 0;
    ObjectCandidate cands[3];

    portENTER_CRITICAL(&g_target_mutex);
    target = g_current_target;
    num_cands = g_num_candidates;
    insp_idx = g_inspected_candidate_idx;
    for (int i = 0; i < 3; i++) {
        cands[i] = g_object_candidates[i];
    }
    portEXIT_CRITICAL(&g_target_mutex);

    BrainTelemetry brain = getBrainTelemetry();
    PersonalityTraits traits = getPersonalityTraits();
    CircadianState circa = getCircadianState();

    bool is_recon_sleeping = (g_recon_state == STATE_SLEEP_RECON);
    float current_fps = is_recon_sleeping ? 0.0f : g_fps_ai;
    bool is_detected = is_recon_sleeping ? false : target.detected;

    snprintf(json, max_len,
        "{\"type\":\"telemetry\",\"detected\":%s,\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,\"cx\":%d,\"cy\":%d,\"err_x\":%.1f,\"err_y\":%.1f,\"conf\":%.2f,\"human_likelihood\":%.2f,\"fps_ai\":%.1f,\"fw\":%d,\"fh\":%d,\"vx\":%.1f,\"vy\":%.1f,\"prox\":%.2f,\"num_cands\":%d,\"insp_idx\":%d,\"c0_cx\":%d,\"c0_cy\":%d,\"c0_w\":%d,\"c0_h\":%d,\"c0_p\":%.1f,\"c1_cx\":%d,\"c1_cy\":%d,\"c1_w\":%d,\"c1_h\":%d,\"c1_p\":%.1f,\"c2_cx\":%d,\"c2_cy\":%d,\"c2_w\":%d,\"c2_h\":%d,\"c2_p\":%.1f,\"expr\":%d,\"expr_name\":\"%s\",\"is_manual\":%s,\"valence\":%.2f,\"arousal\":%.2f,\"curiosity\":%.2f,\"social\":%.2f,\"boredom\":%.2f,\"fatigue\":%.2f,\"mischief\":%.2f,\"thought\":\"%s\",\"interact_s\":%u,\"solitude_s\":%u,\"bonding\":%.2f,\"life_s\":%u,\"mem_count\":%u,\"mem_res\":%.2f,\"mem_expr\":%d,\"heap_free\":%u,\"psram_free\":%u,\"uptime_s\":%lu,\"cpu_mhz\":%d,\"cam_sleep\":%s,\"cam_online\":%s,\"brightness\":%u,\"auto_brightness\":%s,\"personality\":{\"boldness\":%.2f,\"volatility\":%.2f,\"playfulness\":%.2f,\"attachment\":%.2f},\"circadian\":{\"energy\":%.2f,\"mood_offset\":%.2f,\"phase_pct\":%.1f}}",
        is_detected ? "true" : "false",
        target.x, target.y, target.w, target.h,
        target.cx, target.cy,
        target.error_x, target.error_y,
        is_recon_sleeping ? 0.0f : target.confidence,
        is_recon_sleeping ? 0.0f : target.human_likelihood,
        current_fps,
        640, 480,
        target.vx, target.vy,
        target.proximity,
        num_cands, insp_idx,
        cands[0].cx, cands[0].cy, cands[0].w, cands[0].h, cands[0].priority_score,
        cands[1].cx, cands[1].cy, cands[1].w, cands[1].h, cands[1].priority_score,
        cands[2].cx, cands[2].cy, cands[2].w, cands[2].h, cands[2].priority_score,
        (int)g_currentExpr,
        getExpressionName(g_currentExpr),
        isManualExpressionActive() ? "true" : "false",
        getEmotionValence(), getEmotionArousal(),
        brain.drives.curiosity, brain.drives.social, brain.drives.boredom, brain.drives.fatigue, brain.drives.mischief,
        brain.thought_summary,
        brain.interaction_sec, brain.solitude_sec,
        brain.bonding_level, brain.lifetime_sec,
        (unsigned)brain.memory_count, brain.memory_resonance, (int)brain.last_recalled_expr,
        (unsigned)esp_get_free_heap_size(),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        (unsigned long)(millis() / 1000),
        ESP.getCpuFreqMHz(),
        is_recon_sleeping ? "true" : "false",
        "false",
        (unsigned)g_oled_brightness,
        g_auto_brightness_enabled ? "true" : "false",
        traits.boldness, traits.volatility, traits.playfulness, traits.attachment,
        circa.energy_level, circa.mood_baseline, circa.phase_pct
    );
}
