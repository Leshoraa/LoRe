/**
 * @file display_engine.cpp
 * @brief LovyanGFX SSD1306 display driver and frame orchestrator implementation.
 */

#include "src/core/display_engine.h"
#include "src/core/facial_renderer.h"
#include "src/core/ambient_screens.h"
#include "src/core/gaze_engine.h"
#include "src/config/device_config.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include "src/math/kinematics.h"
#include "src/math/affective_engine.h"
#include "src/ai/personality_engine.h"
#include <Arduino.h>
#include <esp_random.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

LGFX lcd;
LGFX_Sprite canvas(&lcd);

volatile bool g_auto_brightness_enabled = DEFAULT_AUTO_BRIGHTNESS;

static ReconState s_prev_recon_state = STATE_ACTIVE;
static bool s_isDoubleBlinkPending = false;
static int s_burn_shift_x = 0;
static int s_burn_shift_y = 0;
static uint32_t s_last_burn_shift_ms = 0;

static AmbientScreenMode s_active_ambient_mode = AMBIENT_NONE;
static uint32_t s_ambient_popup_until_ms = 0;
static uint32_t s_next_random_ambient_check_ms = 0;
static uint8_t s_last_applied_brightness = 0;
static volatile bool s_is_manual_ambient = false;
static volatile AmbientScreenMode s_pending_ambient_mode = AMBIENT_NONE;
static volatile uint32_t s_pending_ambient_duration = 0;

int get_burn_shift_x(void) {
    return s_burn_shift_x;
}

int get_burn_shift_y(void) {
    return s_burn_shift_y;
}

void showBootStatus(const char* line1, const char* line2) {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(4, 20);
    if (line1) lcd.print(line1);
    if (line2) {
        lcd.setCursor(4, 36);
        lcd.print(line2);
    }
}

void setOledBrightnessLive(uint8_t brightness) {
    g_oled_brightness = brightness;
}

void setAutoBrightnessLive(bool enabled) {
    g_auto_brightness_enabled = enabled;
}

void triggerAmbientDisplay(AmbientScreenMode mode, uint32_t duration_ms) {
    s_pending_ambient_mode = mode;
    s_pending_ambient_duration = duration_ms;
    s_is_manual_ambient = true;
}

void triggerClockDisplay(uint32_t duration_ms) {
    triggerAmbientDisplay(AMBIENT_CLOCK, duration_ms);
}

void triggerWeatherDisplay(uint32_t duration_ms) {
    triggerAmbientDisplay(AMBIENT_WEATHER, duration_ms);
}

void triggerNotificationDisplay(const NotificationInfo& notif, uint32_t duration_ms) {
    (void)notif;
    triggerAmbientDisplay(AMBIENT_NOTIFICATION, duration_ms);
}

void triggerNavigationDisplay(const NavigationInfo& nav, uint32_t duration_ms) {
    portENTER_CRITICAL(&g_nav_mutex);
    g_nav_info = nav;
    g_nav_info.active = true;
    g_nav_info.valid = true;
    g_nav_info.updated_ms = millis();
    portEXIT_CRITICAL(&g_nav_mutex);
    triggerAmbientDisplay(AMBIENT_NAVIGATION, duration_ms);
}

void dismissNavigationDisplay(void) {
    portENTER_CRITICAL(&g_nav_mutex);
    g_nav_info.active = false;
    portEXIT_CRITICAL(&g_nav_mutex);
    if (s_active_ambient_mode == AMBIENT_NAVIGATION) {
        s_ambient_popup_until_ms = millis();
    }
}

void oledTask(void* pvParameters) {
    (void)pvParameters;

    s_last_applied_brightness = g_oled_brightness;
    lcd.setBrightness(g_oled_brightness);

    canvas.setColorDepth(1);
    canvas.createSprite(OLED_PANEL_WIDTH_PX, OLED_PANEL_HEIGHT_PX);

    set_facial_canvas(&canvas);
    set_ambient_canvas(&canvas);

    g_currentExpr = EXPR_IDLE;
    drawFace(g_currentExpr, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    while (true) {
        uint32_t frame_start_us = micros();
        unsigned long now = millis();

        static uint32_t s_last_auto_bright_check_ms = 0;
        if (g_auto_brightness_enabled) {
            if (now - s_last_auto_bright_check_ms > 250) {
                s_last_auto_bright_check_ms = now;

                time_t now_sec;
                time(&now_sec);
                struct tm timeinfo;
                bool time_synced = (now_sec > 1700000000);
                bool is_night_time = false;
                if (time_synced) {
                    localtime_r(&now_sec, &timeinfo);
                    is_night_time = (timeinfo.tm_hour >= NIGHT_HOUR_START || timeinfo.tm_hour < NIGHT_HOUR_END);
                }

                float ambient_lum = getAmbientLuminance();
                float target_bright = 5.0f;
                if (ambient_lum <= 25.0f) {
                    target_bright = 5.0f + (ambient_lum / 25.0f) * 30.0f;
                } else if (ambient_lum <= 100.0f) {
                    target_bright = 35.0f + ((ambient_lum - 25.0f) / 75.0f) * 120.0f;
                } else {
                    target_bright = 155.0f + ((ambient_lum - 100.0f) / 155.0f) * 100.0f;
                }

                if (is_night_time) {
                    target_bright = fminf(target_bright * 0.50f, 65.0f);
                }

                uint8_t target_clamped = (uint8_t)fmaxf(3.0f, fminf(255.0f, target_bright));

                if (abs((int)g_oled_brightness - (int)target_clamped) > 1) {
                    g_oled_brightness = (uint8_t)(0.72f * g_oled_brightness + 0.28f * target_clamped);
                    s_last_applied_brightness = g_oled_brightness;
                    lcd.setBrightness(g_oled_brightness);
                }
            }
        } else {
            if (s_last_applied_brightness != g_oled_brightness) {
                s_last_applied_brightness = g_oled_brightness;
                lcd.setBrightness(g_oled_brightness);
            }
        }

        if (g_recon_state != s_prev_recon_state) {
            s_prev_recon_state = g_recon_state;
        }

        if (now - s_last_burn_shift_ms > OLED_BURN_SHIFT_INTERVAL_MS) {
            s_last_burn_shift_ms = now;
            s_burn_shift_x = (int)(esp_random() % 3) - 1;
            s_burn_shift_y = (int)(esp_random() % 3) - 1;
        }

        bool target_engaged = false;
        portENTER_CRITICAL(&g_target_mutex);
        target_engaged = (g_recon_state == STATE_ACTIVE
                          && g_current_target.detected
                          && g_current_target.human_likelihood > HUMAN_LIKELIHOOD_THRESHOLD
                          && (now - g_current_target.last_seen_ms < 600));
        portEXIT_CRITICAL(&g_target_mutex);

        if (s_pending_ambient_mode != AMBIENT_NONE) {
            AmbientScreenMode target = s_pending_ambient_mode;
            uint32_t dur = s_pending_ambient_duration;
            s_pending_ambient_mode = AMBIENT_NONE;
            if (s_active_ambient_mode != target) {
                transitionToAmbient(target, 160.0f);
            }
            s_active_ambient_mode = target;
            s_ambient_popup_until_ms = millis() + dur;
            s_is_manual_ambient = true;
        }

        if (s_next_random_ambient_check_ms == 0) {
            float glance_scale = getPersonalityAmbientGlanceScale();
            uint32_t min_ms = (uint32_t)((float)AMBIENT_INTERVAL_MIN_MS * glance_scale);
            uint32_t max_ms = (uint32_t)((float)AMBIENT_INTERVAL_MAX_MS * glance_scale);
            uint32_t interval_span = (max_ms > min_ms) ? (max_ms - min_ms) : 30000;
            s_next_random_ambient_check_ms = now + (esp_random() % interval_span) + min_ms;
        }

        bool can_spontaneous_glance = !isManualExpressionActive()
                                      && !g_is_transitioning
                                      && (g_currentExpr != EXPR_SHOCK && g_currentExpr != EXPR_OVERLOAD);

        if (can_spontaneous_glance && now >= s_next_random_ambient_check_ms) {
            float glance_scale = getPersonalityAmbientGlanceScale();
            if (target_engaged) glance_scale *= 1.25f;
            uint32_t min_ms = (uint32_t)((float)AMBIENT_INTERVAL_MIN_MS * glance_scale);
            uint32_t max_ms = (uint32_t)((float)AMBIENT_INTERVAL_MAX_MS * glance_scale);
            uint32_t interval_span = (max_ms > min_ms) ? (max_ms - min_ms) : 30000;
            s_next_random_ambient_check_ms = now + (esp_random() % interval_span) + min_ms;

            if (s_ambient_popup_until_ms < now) {
                bool weather_valid = false;
                portENTER_CRITICAL(&g_weather_mutex);
                weather_valid = g_weather_info.valid;
                portEXIT_CRITICAL(&g_weather_mutex);

                bool weather_available = is_weather_enabled() && weather_valid;
                uint32_t roll = esp_random() % 100;
                AmbientScreenMode chosen_mode = AMBIENT_NONE;

                static AmbientScreenMode s_last_spontaneous_mode = AMBIENT_NONE;

                if (weather_available) {
                    if (s_last_spontaneous_mode == AMBIENT_WEATHER || roll < 75) {
                        chosen_mode = AMBIENT_CLOCK;
                    } else {
                        chosen_mode = AMBIENT_WEATHER;
                    }
                } else {
                    chosen_mode = AMBIENT_CLOCK;
                }

                s_last_spontaneous_mode = chosen_mode;

                if (chosen_mode != AMBIENT_NONE) {
                    s_is_manual_ambient = false;
                    transitionToAmbient(chosen_mode, 160.0f);
                    s_active_ambient_mode = chosen_mode;
                    uint32_t duration_span = (AMBIENT_POPUP_DURATION_MAX_MS > AMBIENT_POPUP_DURATION_MIN_MS) ? (AMBIENT_POPUP_DURATION_MAX_MS - AMBIENT_POPUP_DURATION_MIN_MS) : 2000;
                    uint32_t random_duration_ms = (esp_random() % duration_span) + AMBIENT_POPUP_DURATION_MIN_MS;
                    s_ambient_popup_until_ms = now + random_duration_ms;
                }
            }
        }

        if (now < s_ambient_popup_until_ms && s_active_ambient_mode != AMBIENT_NONE) {
            g_animFrame += 0.04f;
            if (s_active_ambient_mode == AMBIENT_CLOCK) {
                drawClockScreen(g_animFrame);
            } else if (s_active_ambient_mode == AMBIENT_WEATHER) {
                WeatherInfo local_weather;
                portENTER_CRITICAL(&g_weather_mutex);
                local_weather = g_weather_info;
                portEXIT_CRITICAL(&g_weather_mutex);
                drawWeatherScreen(local_weather, g_animFrame);
            } else if (s_active_ambient_mode == AMBIENT_NOTIFICATION) {
                NotificationInfo local_notif;
                portENTER_CRITICAL(&g_notification_mutex);
                local_notif = g_notification_info;
                portEXIT_CRITICAL(&g_notification_mutex);
                drawNotificationScreen(local_notif, g_animFrame);
            } else if (s_active_ambient_mode == AMBIENT_NAVIGATION) {
                NavigationInfo local_nav;
                portENTER_CRITICAL(&g_nav_mutex);
                local_nav = g_nav_info;
                portEXIT_CRITICAL(&g_nav_mutex);
                if (!local_nav.active) {
                    s_ambient_popup_until_ms = now;
                } else {
                    drawNavigationScreen(local_nav, g_animFrame);
                }
            }
        } else {
            if (s_active_ambient_mode != AMBIENT_NONE) {
                AmbientScreenMode prev_mode = s_active_ambient_mode;
                s_active_ambient_mode = AMBIENT_NONE;
                s_is_manual_ambient = false;
                transitionFromAmbientToFace(prev_mode, g_currentExpr, 140.0f);
            }
            Expression prevExpr = g_currentExpr;
            updateBiologicalMoodEngine();

            if (g_currentExpr != prevExpr) {
                frame_start_us = micros();
            } else {
                updateGazeSystem();

                bool canBlink = (g_currentExpr != EXPR_OVERLOAD && g_currentExpr != EXPR_SAD && g_currentExpr != EXPR_JOY);

                if (!canBlink) {
                    g_blinkState = BLINK_IDLE_STATE;
                    g_blinkEyeHeight = 1.0f;
                } else {
                    if (g_blinkState == BLINK_IDLE_STATE) {
                        if (g_nextBlinkTime == 0) {
                            uint32_t initInterval = getPersonalityBlinkInterval();
                            if (g_currentExpr == EXPR_ANGRY) initInterval = (uint32_t)(initInterval * 1.20f);
                            g_nextBlinkTime = now + initInterval;
                        }
                        if (now >= g_nextBlinkTime) {
                            g_blinkState = BLINK_CLOSING_STATE;
                            g_nextBlinkTime = now;
                            if (s_isDoubleBlinkPending) {
                                s_isDoubleBlinkPending = false;
                            } else if ((esp_random() % 100) < getPersonalityDoubleBlinkChance()) {
                                s_isDoubleBlinkPending = true;
                            }
                        }
                    }

                    if (g_blinkState == BLINK_CLOSING_STATE) {
                        float elapsed = (float)(now - g_nextBlinkTime);
                        float duration = (g_currentExpr == EXPR_ANGRY) ? 35.0f : 50.0f;
                        if (elapsed >= duration) {
                            g_blinkEyeHeight = 0.0f;
                            g_blinkState = BLINK_OPENING_STATE;
                            g_nextBlinkTime = now;
                        } else {
                            float t = elapsed / duration;
                            g_blinkEyeHeight = blinkCloseEase(t);
                        }
                    } else if (g_blinkState == BLINK_OPENING_STATE) {
                        float elapsed = (float)(now - g_nextBlinkTime);
                        float duration = (g_currentExpr == EXPR_ANGRY) ? 80.0f : 110.0f;
                        if (elapsed >= duration) {
                            g_blinkEyeHeight = 1.0f;
                            g_blinkState = BLINK_IDLE_STATE;
                            if (s_isDoubleBlinkPending) {
                                g_nextBlinkTime = now + 120;
                            } else {
                                uint32_t interval = getPersonalityBlinkInterval();
                                if (g_currentExpr == EXPR_ANGRY) interval = (uint32_t)(interval * 1.20f);
                                g_nextBlinkTime = now + interval;
                            }
                        } else {
                            float t = elapsed / duration;
                            g_blinkEyeHeight = blinkOpenEase(t);
                        }
                    } else {
                        g_blinkEyeHeight = 1.0f;
                    }
                }

                if (g_currentExpr == EXPR_OVERLOAD || g_currentExpr == EXPR_SAD) {
                    g_animFrame += 0.025f;
                }

                drawFace(g_currentExpr, g_blinkEyeHeight, g_currentOffsetX, g_currentOffsetY, g_animFrame, g_currentVergence, g_currentEyeScale);
            }
        }

        uint32_t frame_budget_us = (g_recon_state == STATE_SLEEP_RECON) ? FRAME_BUDGET_SLEEP_US : FRAME_BUDGET_ACTIVE_US;
        uint32_t frame_elapsed_us = micros() - frame_start_us;
        if (frame_elapsed_us < frame_budget_us) {
            uint32_t wait_us = frame_budget_us - frame_elapsed_us;
            if (wait_us > 2000) {
                vTaskDelay(pdMS_TO_TICKS((wait_us - 500) / 1000));
            }
            uint32_t remaining_us = frame_budget_us - (micros() - frame_start_us);
            if (remaining_us > 0 && remaining_us < 2000) {
                delayMicroseconds(remaining_us);
            }
        } else {
            vTaskDelay(1);
        }
    }
}
