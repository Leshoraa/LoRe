/**
 * @file gaze_engine.cpp
 * @brief Autonomous gaze coordination, stimulus integration, and Core 0 power management implementation.
 */

#include "src/core/gaze_engine.h"
#include <Arduino.h>
#include <esp_random.h>
#include <math.h>

/* Cross-Core Synchronization Handles */
portMUX_TYPE g_target_mutex = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE g_weather_mutex = portMUX_INITIALIZER_UNLOCKED;

/* Global Cross-Core State Variables */
TrackTarget g_current_target;
ObjectCandidate g_object_candidates[MAX_OBJECT_CANDIDATES];
int g_num_candidates = 0;
int g_inspected_candidate_idx = 0;
volatile ReconState g_recon_state = STATE_ACTIVE;
volatile float g_fps_ai = 60.0f;
volatile uint32_t g_last_web_activity_ms = 0;
volatile uint8_t g_oled_brightness = OLED_DEFAULT_BRIGHTNESS;
WeatherInfo g_weather_info;

static uint32_t s_virtual_target_until_ms = 0;
static uint32_t s_last_active_activity_ms = 0;

void initGazeEngine(void) {
    portENTER_CRITICAL(&g_target_mutex);
    g_current_target.detected = false;
    g_current_target.x = 0;
    g_current_target.y = 0;
    g_current_target.w = 0;
    g_current_target.h = 0;
    g_current_target.cx = 0;
    g_current_target.cy = 0;
    g_current_target.error_x = 0.0f;
    g_current_target.error_y = 0.0f;
    g_current_target.confidence = 0.0f;
    g_current_target.human_likelihood = 0.0f;
    g_current_target.total_energy = 0.0f;
    g_current_target.vx = 0.0f;
    g_current_target.vy = 0.0f;
    g_current_target.proximity = 0.0f;
    g_current_target.last_seen_ms = 0;
    portEXIT_CRITICAL(&g_target_mutex);

    for (int i = 0; i < MAX_OBJECT_CANDIDATES; i++) {
        g_object_candidates[i].active = false;
        g_object_candidates[i].cx = 0;
        g_object_candidates[i].cy = 0;
        g_object_candidates[i].w = 0;
        g_object_candidates[i].h = 0;
        g_object_candidates[i].priority_score = 0.0f;
        g_object_candidates[i].error_x = 0.0f;
        g_object_candidates[i].error_y = 0.0f;
        g_object_candidates[i].skin_px = 0;
        g_object_candidates[i].motion_energy = 0.0f;
        g_object_candidates[i].proximity = 0.0f;
    }

    g_weather_info.valid = false;
    g_weather_info.temperature = 0.0f;
    g_weather_info.humidity = 0;
    g_weather_info.weather_code = 0;
    g_weather_info.city[0] = '\0';
    g_weather_info.condition[0] = '\0';
    g_weather_info.last_sync_ms = 0;
    g_weather_info.sunrise_hour = DEFAULT_SUNRISE_HOUR;
    g_weather_info.sunrise_min = DEFAULT_SUNRISE_MIN;
    g_weather_info.sunset_hour = DEFAULT_SUNSET_HOUR;
    g_weather_info.sunset_min = DEFAULT_SUNSET_MIN;
    g_weather_info.sun_times_valid = false;

    g_notification_info.active = false;
    g_notification_info.title[0] = '\0';
    g_notification_info.message[0] = '\0';
    g_notification_info.app[0] = '\0';
    g_notification_info.received_ms = 0;

    g_nav_info.active = false;
    g_nav_info.valid = false;
    g_nav_info.icon = NAV_ICON_NONE;
    g_nav_info.distance[0] = '\0';
    g_nav_info.instruction[0] = '\0';
    g_nav_info.street[0] = '\0';
    g_nav_info.eta[0] = '\0';
    g_nav_info.duration[0] = '\0';
    g_nav_info.total_dist[0] = '\0';
    g_nav_info.updated_ms = 0;

    s_last_active_activity_ms = millis();
    setCpuFrequencyMhz(CPU_FREQ_ACTIVE_MHZ);
    LORE_LOG_INF("GAZE", "Autonomous gaze engine initialized in ACTIVE mode");
}

void setVirtualTarget(float normX, float normY, float duration_ms) {
    normX = constrain(normX, -1.0f, 1.0f);
    normY = constrain(normY, -1.0f, 1.0f);

    uint32_t now = millis();
    portENTER_CRITICAL(&g_target_mutex);
    g_current_target.detected = true;
    g_current_target.error_x = normX * 100.0f;
    g_current_target.error_y = normY * 100.0f;
    g_current_target.confidence = 0.95f;
    g_current_target.human_likelihood = 0.85f;
    g_current_target.proximity = 0.70f;
    g_current_target.total_energy = 25.0f;
    g_current_target.last_seen_ms = now;
    portEXIT_CRITICAL(&g_target_mutex);

    s_virtual_target_until_ms = now + (uint32_t)duration_ms;
    s_last_active_activity_ms = now;
    if (g_recon_state != STATE_ACTIVE) {
        setCpuFrequencyMhz(CPU_FREQ_ACTIVE_MHZ);
        g_recon_state = STATE_ACTIVE;
    }
}

void gazeTask(void *pvParameters) {
    (void)pvParameters;
    LORE_LOG_INF("GAZE", "Core 0 Gaze & Power Management Task started");

    while (true) {
        uint32_t now = millis();

        /* Handle virtual target expiration: seamlessly return to autonomous saccades */
        if (s_virtual_target_until_ms > 0 && now >= s_virtual_target_until_ms) {
            s_virtual_target_until_ms = 0;
            portENTER_CRITICAL(&g_target_mutex);
            g_current_target.detected = false;
            g_current_target.confidence = 0.0f;
            g_current_target.human_likelihood = 0.0f;
            portEXIT_CRITICAL(&g_target_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

float getAmbientLuminance(void) {
    return 120.0f;
}
