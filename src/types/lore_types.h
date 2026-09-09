/**
 * @file lore_types.h
 * @brief Common data structures, enumerations, and cross-core state declarations for LoRe.
 */

#ifndef LORE_TYPES_H
#define LORE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
typedef void* SemaphoreHandle_t;
#define portENTER_CRITICAL(m) ((void)0)
#define portEXIT_CRITICAL(m) ((void)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum Expression
 * @brief Discrete 2D facial emotional expressions rendered on the OLED display.
 */
typedef enum {
    EXPR_IDLE = 0,
    EXPR_HAPPY = 1,
    EXPR_ANGRY = 2,
    EXPR_SAD = 3,
    EXPR_SURPRISED = 4,
    EXPR_SUSPICIOUS = 5,
    EXPR_CURIOUS = 6,
    EXPR_MISCHIEF = 7,
    EXPR_SLEEPY = 8,
    EXPR_COOL = 9,
    EXPR_DIZZY = 10,
    EXPR_CRYING = 11
} Expression;

#define NUM_EXPRESSIONS 12

/**
 * @struct OcularSomaState
 * @brief Continuous parametric morphology and hardware actuation state for LoRe's physical soma.
 */
typedef struct {
    float left_x, left_y;         /* Left eye center coordinates */
    float right_x, right_y;       /* Right eye center coordinates */
    float left_w, left_h;         /* Left eye width and height */
    float right_w, right_h;       /* Right eye width and height */
    float left_n, right_n;        /* Superellipse Lamé exponent (2.0 = ellipse, 4.0..5.0 = squircle, <2.0 = astroid) */
    float stroke_thickness;       /* 0.0f = solid filled, 1.0f..4.0f = hollow outline border width */
    float tilt_left, tilt_right;  /* Eye tilt slant angle in radians */
    float brow_tilt_left, brow_tilt_right;   /* Upper eyelid / eyebrow slant angle in radians */
    float cheek_tilt_left, cheek_tilt_right; /* Lower eyelid / cheek slant angle in radians */
    float upper_lid_left, upper_lid_right;   /* Upper eyelid droop progress [0.0 = open, 1.0 = closed] */
    float lower_lid_left, lower_lid_right;   /* Lower eyelid push-up [0.0 = baseline, 1.0 = high cheek smile] */
    float palpebral_aperture;     /* Upper/lower eyelid opening factor [0.0 = fully closed slit, 1.0 = fully open] */
    uint8_t hardware_contrast;    /* SSD1306 physical OLED contrast brightness [0..255] */
    float nystagmus_x, nystagmus_y; /* Sub-pixel physiological micro-tremor */
} OcularSomaState;

/**
 * @enum BlinkState
 * @brief Non-blocking state machine states for eyelid blinking animations.
 */
typedef enum {
    BLINK_IDLE_STATE = 0,
    BLINK_CLOSING_STATE,
    BLINK_CLOSED_STATE,
    BLINK_OPENING_STATE
} BlinkState;

/**
 * @enum ReconState
 * @brief Operational activity and sleep state for dynamic power management.
 */
typedef enum {
    STATE_ACTIVE = 0,       /* Active animation and responsiveness @ 240 MHz CPU */
    STATE_SLEEP_RECON,      /* Low-power standby, CPU @ 80 MHz, calm resting saccades */
    STATE_SAMPLING          /* Periodic wake-up verification pulse */
} ReconState;

/**
 * @struct TrackTarget
 * @brief Target coordinates and dynamics passed across FreeRTOS cores.
 * Access must be protected with g_target_mutex spinlock.
 */
typedef struct {
    bool detected;
    int x;
    int y;
    int w;
    int h;
    int cx;
    int cy;
    float error_x;
    float error_y;
    float confidence;
    float human_likelihood;  /* Stimulus likelihood [0.0, 1.0] gating bonding and social drive */
    float total_energy;
    float vx;
    float vy;
    float proximity;        /* Normalized proximity Z in range [0.0 (far), 1.0 (near)] */
    uint32_t last_seen_ms;
} TrackTarget;

#define MAX_OBJECT_CANDIDATES 3

/**
 * @struct ObjectCandidate
 * @brief Target candidate descriptor for spatial interest tracking.
 */
typedef struct {
    bool active;
    int cx;
    int cy;
    int w;
    int h;
    float priority_score;
    float error_x;
    float error_y;
    int skin_px;
    float motion_energy;
    float proximity;
} ObjectCandidate;

/**
 * @struct WeatherInfo
 * @brief Current weather observation payload for OLED visualization and Web telemetry.
 */
typedef struct {
    float temperature;
    int humidity;
    int weather_code;
    char city[32];
    char condition[24];
    bool valid;
    uint32_t last_sync_ms;
    uint8_t sunrise_hour;
    uint8_t sunrise_min;
    uint8_t sunset_hour;
    uint8_t sunset_min;
    bool sun_times_valid;
} WeatherInfo;

/**
 * @struct NotificationInfo
 * @brief Phone notification payload for OLED visualization and Web telemetry.
 */
typedef struct {
    char title[36];     /* e.g. "WhatsApp - Budi" or "Telegram" */
    char message[96];   /* Notification message text */
    char app[16];       /* Application identifier e.g. "WhatsApp", "SMS", "Gmail" */
    uint32_t received_ms;
    bool active;
} NotificationInfo;

/**
 * @enum NavIconType
 * @brief Turn-by-turn navigation maneuver icon types for OLED visualization.
 */
typedef enum {
    NAV_ICON_NONE = 0,
    NAV_ICON_STRAIGHT,
    NAV_ICON_TURN_RIGHT,
    NAV_ICON_TURN_LEFT,
    NAV_ICON_SLIGHT_RIGHT,
    NAV_ICON_SLIGHT_LEFT,
    NAV_ICON_SHARP_RIGHT,
    NAV_ICON_SHARP_LEFT,
    NAV_ICON_UTURN,
    NAV_ICON_ROUNDABOUT,
    NAV_ICON_ARRIVE
} NavIconType;

/**
 * @struct NavigationInfo
 * @brief Real-time turn-by-turn navigation payload synchronized via BLE.
 */
typedef struct {
    NavIconType icon;     /* Directional maneuver icon type */
    char distance[16];    /* Distance to next turn/step, e.g. "200 m", "1.5 km" */
    char instruction[32]; /* Maneuver instruction text, e.g. "Turn right", "Head north" */
    char street[48];      /* Target roadway or destination, e.g. "Grand Ave" */
    char eta[16];         /* Estimated time of arrival, e.g. "14:49" */
    char duration[16];    /* Estimated remaining duration, e.g. "15 min" */
    char total_dist[16];  /* Total remaining route distance, e.g. "2.9 km" */
    uint32_t updated_ms;  /* Timestamp of last update in milliseconds */
    bool active;          /* Active route status flag */
    bool valid;           /* Data validity flag */
} NavigationInfo;

/* Cross-Core Synchronization Handles */
extern portMUX_TYPE g_target_mutex;
extern portMUX_TYPE g_weather_mutex;
extern portMUX_TYPE g_notification_mutex;
extern portMUX_TYPE g_nav_mutex;

/* Global Cross-Core State Variables */
extern TrackTarget g_current_target;
extern ObjectCandidate g_object_candidates[MAX_OBJECT_CANDIDATES];
extern int g_num_candidates;
extern int g_inspected_candidate_idx;
extern volatile ReconState g_recon_state;
extern volatile float g_fps_ai;
extern volatile uint32_t g_last_web_activity_ms;
extern volatile uint8_t g_oled_brightness;
extern WeatherInfo g_weather_info;
extern NotificationInfo g_notification_info;
extern NavigationInfo g_nav_info;

#ifdef __cplusplus
}
#endif

#endif /* LORE_TYPES_H */
