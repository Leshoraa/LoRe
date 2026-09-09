/**
 * @file lore_config.h
 * @brief System hardware definitions, power profiles, and logging macros for LoRe.
 *
 * Target Platform: ESP32-S3 SuperMini
 * Display: 0.96" I2C OLED (SSD1306, 128x64)
 * Dual-Core FreeRTOS Architecture:
 *   Core 0: Network, HTTP Server, BLE Companion, Ambient Services, DFS
 *   Core 1: 60 FPS Biomechanical Gaze Kinematics & 1-Bit LGFX Sprite Engine
 */

#ifndef LORE_CONFIG_H
#define LORE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(__has_include)
    #if __has_include("env.h")
        #include "env.h"
    #elif __has_include("src/config/env.h")
        #include "src/config/env.h"
    #elif __has_include("include/env.h")
        #include "include/env.h"
    #elif __has_include("env.example.h")
        #include "env.example.h"
    #elif __has_include("src/config/env.example.h")
        #include "src/config/env.example.h"
    #elif __has_include("include/env.example.h")
        #include "include/env.example.h"
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 0
#endif

#if defined(ENABLE_SERIAL_DEBUG) && (ENABLE_SERIAL_DEBUG == 1)
    #define LORE_LOG_DBG(tag, fmt, ...) printf("[%s][DBG] %s:%d: " fmt "\n", tag, __func__, __LINE__, ##__VA_ARGS__)
    #define LORE_LOG_INF(tag, fmt, ...) printf("[%s][INF] " fmt "\n", tag, ##__VA_ARGS__)
#else
    #define LORE_LOG_DBG(tag, fmt, ...) ((void)0)
    #define LORE_LOG_INF(tag, fmt, ...) ((void)0)
#endif
#define LORE_LOG_ERR(tag, fmt, ...)     fprintf(stderr, "[%s][ERR] %s:%d: " fmt "\n", tag, __func__, __LINE__, ##__VA_ARGS__)

/* Hardware Pin Configuration for ESP32-S3 SuperMini */
#define PIN_OLED_SCL                    3
#define PIN_OLED_SDA                    12
#define OLED_I2C_PORT                   0
#define OLED_I2C_FREQ_WRITE_HZ          1000000  /* 1.0 MHz Fast-Mode Plus I2C Overclock */
#define OLED_I2C_FREQ_READ_HZ           400000
#define OLED_PANEL_WIDTH_PX             128
#define OLED_PANEL_HEIGHT_PX            64
#define OLED_DEFAULT_BRIGHTNESS         128
#define DEFAULT_AUTO_BRIGHTNESS         false
#define NIGHT_HOUR_START                18       /* 18:00 (6 PM) fallback night dimming curve */
#define NIGHT_HOUR_END                  6        /* 06:00 (6 AM) fallback night dimming curve */
#define OLED_BURN_SHIFT_INTERVAL_MS     35000    /* 35 seconds periodic anti-burn pixel orbit shift */
#define MANUAL_EXPR_TIMEOUT_MS          60000    /* 60 seconds auto-revert timeout for manual expressions */

/* OLED Deep Sleep Anti-Burn-In & Dark Night Window */
#define OLED_DEEP_SLEEP_DEFAULT_ENABLED true
#define OLED_DEEP_SLEEP_START_HOUR      1        /* 01:00 (1 AM) */
#define OLED_DEEP_SLEEP_START_MIN       0
#define OLED_DEEP_SLEEP_END_HOUR        5        /* 05:30 (5:30 AM) default wake time */
#define OLED_DEEP_SLEEP_END_MIN         30
#define OLED_DEEP_SLEEP_WAKE_TIMEOUT_MS 15000    /* 15s momentary wake on touch/interaction */

/* Astronomical Sun Defaults (Fallback if offline or un-synced) */
#define DEFAULT_SUNRISE_HOUR            6
#define DEFAULT_SUNRISE_MIN             0
#define DEFAULT_SUNSET_HOUR             18
#define DEFAULT_SUNSET_MIN              0

/* Default Station (STA) Credentials (overridden by env.h) */
#ifndef WIFI_STA_DEFAULT_SSID
#define WIFI_STA_DEFAULT_SSID           ""
#endif
#ifndef WIFI_STA_DEFAULT_PASS
#define WIFI_STA_DEFAULT_PASS           ""
#endif

/* Fallback Access Point & Captive Portal */
#ifndef WIFI_FALLBACK_AP_SSID
#define WIFI_FALLBACK_AP_SSID           "LoRe-AP"
#endif
#ifndef WIFI_FALLBACK_AP_PASS
#define WIFI_FALLBACK_AP_PASS           "12345678"
#endif

/* Push Notifications & Ntfy.sh Cloud Push Integration */
#ifndef NTFY_DEFAULT_TOPIC
#define NTFY_DEFAULT_TOPIC              "lore_notif_default"
#endif
#define NOTIFICATION_POPUP_DURATION_MS  6000     /* 6 seconds notification display on OLED */
#define NTFY_POLL_INTERVAL_MS           2500     /* 2.5 seconds periodic stream/poll interval */

/* Bluetooth Low Energy (BLE) Companion Notifications & Telemetry */
#ifndef BLE_DEVICE_NAME_DEFAULT
#define BLE_DEVICE_NAME_DEFAULT         "LoRe-SuperMini"
#endif
#define BLE_NUS_SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_NUS_CHAR_RX_UUID            "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_NUS_CHAR_TX_UUID            "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/* Weather Configuration & Open-Meteo Integration */
#define WEATHER_DEFAULT_CITY            "Jakarta"
#define WEATHER_DEFAULT_LAT             -6.2088f
#define WEATHER_DEFAULT_LON             106.8456f
#define AMBIENT_POPUP_DURATION_MIN_MS   3500     /* 3.5 seconds brief glance on OLED */
#define AMBIENT_POPUP_DURATION_MAX_MS   5500     /* 5.5 seconds maximum glance on OLED */
#define AMBIENT_INTERVAL_MIN_MS         30000    /* 30 seconds minimum between spontaneous glances */
#define AMBIENT_INTERVAL_MAX_MS         75000    /* 75 seconds (~1.25 mins) maximum between glances */
#define WEATHER_POPUP_DURATION_MS       5000     /* 5 seconds manual trigger display on OLED */
#define WEATHER_FETCH_INTERVAL_MS       1800000  /* 30 minutes periodic sync */

/* Dynamic Frequency Scaling (DFS) Configuration */
#define CPU_FREQ_ACTIVE_MHZ             240      /* Full compute frequency */
#define CPU_FREQ_SLEEP_MHZ              80       /* Minimum frequency keeping Wi-Fi stack stable */
#define ACTIVE_STATE_TIMEOUT_MS         5000     /* Standby inactivity timeout */

/* Real-Time Execution Budgets */
#define TARGET_FPS_ACTIVE               60.0f
#define TARGET_FPS_SLEEP                30.0f
#define FRAME_BUDGET_ACTIVE_US          16666    /* 16.66 ms frame period (60 FPS) */
#define FRAME_BUDGET_SLEEP_US           33333    /* 33.33 ms frame period (30 FPS) */

/* Biomechanical Dynamic Modeling Parameters */
#define GAZE_NATURAL_FREQUENCY_RAD_S    32.0f    /* Ocular natural frequency omega_n */
#define GAZE_DAMPING_RATIO              0.72f    /* Underdamped compliance zeta: optimal 4.3% biological bounce */
#define BOUNCE_SQUASH_STRETCH_GAIN      0.14f    /* Volume-conserving biological squash/stretch amplitude */
#define GLISSADE_REBOUND_GAIN           0.045f   /* Saccadic landing ocular micro-glissade rebound */
#define GAZE_GAIN_X                     1.75f    /* Horizontal tracking gain */
#define GAZE_GAIN_Y                     1.45f    /* Vertical tracking gain */
#define GAZE_DEADBAND_RADIUS_PX         1.35f    /* Sub-pixel noise gate threshold */
#define SACCADE_D0_MS                   20.0f    /* Flash & Hogan base duration */
#define SACCADE_K_MS_PER_DEG            2.5f     /* Flash & Hogan duration slope */
#define PX_TO_DEG_FACTOR                1.714f   /* 30 deg / 17.5 px */

/* Affective Dynamics Parameters */
#define AFFECTIVE_TAU_VALENCE_S         6.0f     /* Langevin valence relaxation constant */
#define AFFECTIVE_TAU_AROUSAL_S         4.5f     /* Langevin arousal relaxation constant */

/* Autonomous Ocular Soma & Morphology Parameters (Lamé Superellipse) */
#define SOMA_CANONICAL_EYE_WIDTH_PX     32.0f    /* Baseline horizontal ocular dimension */
#define SOMA_CANONICAL_EYE_HEIGHT_PX    30.0f    /* Baseline vertical ocular dimension */
#define SOMA_CANONICAL_SQUIRCLE_N       4.2f     /* Signature squircle curvature exponent */
#define SOMA_CANONICAL_STROKE_WIDTH     0.0f     /* 0.0f = solid filled, >0.0f = perimeter outline */
#define SOMA_CANONICAL_LEFT_X           40.0f    /* Primary left eye orbital center X */
#define SOMA_CANONICAL_RIGHT_X          88.0f    /* Primary right eye orbital center X */
#define SOMA_CANONICAL_CENTER_Y         32.0f    /* Primary orbital center Y */

/* Network Web Server Port */
#define HTTP_PORT_WEB_CONTROL           80

/* Autonomous Gaze & Stimulus Weights */
#define HUMAN_LIKELIHOOD_THRESHOLD      0.55f    /* Minimum stimulus score for social drive */

/* Personality Trait Defaults */
#define PERSONALITY_DEFAULT_BOLDNESS    0.55f    /* [0,1]: Shy/avoidant (0) to confident (1) */
#define PERSONALITY_DEFAULT_VOLATILITY  0.40f    /* [0,1]: Stable (0) to reactive (1) */
#define PERSONALITY_DEFAULT_PLAYFULNESS 0.65f    /* [0,1]: Stoic (0) to playful (1) */
#define PERSONALITY_DEFAULT_ATTACHMENT  0.50f    /* [0,1]: Aloof (0) to attached (1) */

/* Circadian Rhythm Configuration */
#define CIRCADIAN_CYCLE_PERIOD_MS       21600000 /* 6 hours per internal energy cycle */

/* Firmware Version */
#define LORE_FIRMWARE_VERSION           "1.0.0"

#ifdef __cplusplus
}
#endif

#endif /* LORE_CONFIG_H */
