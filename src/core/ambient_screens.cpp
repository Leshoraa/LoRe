/**
 * @file ambient_screens.cpp
 * @brief Clock, Weather, Phone Notification, and Navigation HUD ambient screen rendering implementation.
 */

#include "src/core/ambient_screens.h"
#include "src/core/display_engine.h"
#include "src/core/facial_renderer.h"
#include "src/core/gaze_engine.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include "src/math/kinematics.h"
#include "src/math/affective_engine.h"
#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

static LGFX_Sprite* s_ambient_canvas = &canvas;

void set_ambient_canvas(LGFX_Sprite* p_canvas) {
    if (p_canvas) {
        s_ambient_canvas = p_canvas;
    }
}

void renderClockToCanvas(float animFrame, int offsetY) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    time_t now_sec;
    time(&now_sec);
    struct tm timeinfo;
    bool time_synced = (now_sec > 1700000000);
    if (time_synced) {
        localtime_r(&now_sec, &timeinfo);
    } else {
        memset(&timeinfo, 0, sizeof(timeinfo));
    }

    char hm_buf[8];
    if (time_synced) {
        bool colon_on = (timeinfo.tm_sec % 2 == 0) || ((int)(animFrame * 2.5f) % 2 == 0);
        snprintf(hm_buf, sizeof(hm_buf), "%02d%c%02d", timeinfo.tm_hour, colon_on ? ':' : ' ', timeinfo.tm_min);
    } else {
        bool colon_on = ((int)(animFrame * 2.5f) % 2 == 0);
        snprintf(hm_buf, sizeof(hm_buf), "--%c--", colon_on ? ':' : ' ');
    }

    int time_w = 5 * 12; /* Size 2: 12 px per char * 5 = 60 px */
    int time_x = (OLED_PANEL_WIDTH_PX - time_w) / 2;
    cv.setTextSize(2);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.setCursor(time_x, 29 + offsetY);
    cv.print(hm_buf);

    cv.setTextSize(1);
    char date_str[24];
    if (time_synced) {
        static const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        snprintf(date_str, sizeof(date_str), "%s, %d %s",
            days[timeinfo.tm_wday % 7],
            timeinfo.tm_mday,
            months[timeinfo.tm_mon % 12]
        );
    } else {
        snprintf(date_str, sizeof(date_str), "Syncing NTP...");
    }
    int date_w = strlen(date_str) * 6;
    int date_x = (OLED_PANEL_WIDTH_PX - date_w) / 2;
    if (date_x < 0) date_x = 0;
    cv.setCursor(date_x, 49 + offsetY);
    cv.print(date_str);
}

void drawClockScreen(float animFrame) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.fillScreen(TFT_BLACK);
    int burn_y = get_burn_shift_y();

    /* Hybrid top zone: Living companion eyes */
    drawMiniFace(EXPR_IDLE, g_blinkEyeHeight, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);
    cv.drawFastHLine(6, 25 + burn_y, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);

    /* Hybrid bottom zone: Digital Clock */
    renderClockToCanvas(animFrame, burn_y);
    cv.pushSprite(0, 0);
}

void renderWeatherToCanvas(const WeatherInfo& weather, float animFrame, int offsetY) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    int code = weather.weather_code;
    int icx = 22;
    int icy = 42 + offsetY;

    if (code == 0 || code == 1) {
        cv.fillCircle(icx, icy, 5, TFT_WHITE);
        for (int i = 0; i < 6; i++) {
            float angle = (float)i * (2.0f * (float)M_PI / 6.0f) + animFrame * 0.2f;
            int x1 = icx + (int)roundf(cosf(angle) * 7.0f);
            int y1 = icy + (int)roundf(sinf(angle) * 7.0f);
            int x2 = icx + (int)roundf(cosf(angle) * 10.0f);
            int y2 = icy + (int)roundf(sinf(angle) * 10.0f);
            cv.drawLine(x1, y1, x2, y2, TFT_WHITE);
        }
    } else if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
        cv.fillCircle(icx - 5, icy - 3, 3, TFT_WHITE);
        cv.fillCircle(icx, icy - 5, 5, TFT_WHITE);
        cv.fillCircle(icx + 5, icy - 3, 3, TFT_WHITE);
        cv.fillRect(icx - 8, icy - 3, 16, 4, TFT_WHITE);

        int dropShift = ((int)(animFrame * 6.0f)) % 4;
        for (int r = -5; r <= 5; r += 5) {
            int rx = icx + r;
            int ry = icy + 3 + dropShift;
            cv.drawLine(rx, ry, rx - 1, ry + 2, TFT_WHITE);
        }
    } else if (code >= 95 && code <= 99) {
        cv.fillCircle(icx - 5, icy - 3, 3, TFT_WHITE);
        cv.fillCircle(icx, icy - 5, 5, TFT_WHITE);
        cv.fillCircle(icx + 5, icy - 3, 3, TFT_WHITE);
        cv.fillRect(icx - 8, icy - 3, 16, 4, TFT_WHITE);

        cv.drawLine(icx, icy + 1, icx - 2, icy + 5, TFT_WHITE);
        cv.drawLine(icx - 2, icy + 5, icx + 1, icy + 5, TFT_WHITE);
        cv.drawLine(icx + 1, icy + 5, icx - 1, icy + 9, TFT_WHITE);
    } else if (code == 45 || code == 48) {
        cv.drawFastHLine(icx - 9, icy - 3, 18, TFT_WHITE);
        cv.drawFastHLine(icx - 6, icy, 12, TFT_WHITE);
        cv.drawFastHLine(icx - 9, icy + 3, 18, TFT_WHITE);
    } else {
        if (code == 2) {
            cv.drawCircle(icx + 6, icy - 6, 2, TFT_WHITE);
        }
        cv.fillCircle(icx - 5, icy - 2, 4, TFT_WHITE);
        cv.fillCircle(icx, icy - 5, 6, TFT_WHITE);
        cv.fillCircle(icx + 5, icy - 2, 4, TFT_WHITE);
        cv.fillRect(icx - 8, icy - 2, 16, 5, TFT_WHITE);
    }

    char temp_buf[16];
    if (weather.valid) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f", weather.temperature);
    } else {
        snprintf(temp_buf, sizeof(temp_buf), "--.-");
    }

    cv.setTextSize(2);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.setCursor(44, 29 + offsetY);
    cv.print(temp_buf);
    int deg_x = 44 + strlen(temp_buf) * 12 + 1;
    cv.drawCircle(deg_x + 2, 30 + offsetY, 1, TFT_WHITE);
    cv.setTextSize(1);
    cv.setCursor(deg_x + 6, 33 + offsetY);
    cv.print("C");

    char cond_buf[32];
    if (weather.valid) {
        if (weather.sun_times_valid && ((int)animFrame % 4 >= 2)) {
            snprintf(cond_buf, sizeof(cond_buf), "^%02d:%02d v%02d:%02d",
                weather.sunrise_hour, weather.sunrise_min,
                weather.sunset_hour, weather.sunset_min
            );
        } else {
            snprintf(cond_buf, sizeof(cond_buf), "%s  %d%%",
                weather.city[0] != '\0' ? weather.city : "Weather",
                weather.humidity
            );
        }
    } else {
        snprintf(cond_buf, sizeof(cond_buf), "Updating...");
    }
    cv.setTextSize(1);
    cv.setCursor(44, 49 + offsetY);
    cv.print(cond_buf);
}

void drawWeatherScreen(const WeatherInfo& weather, float animFrame) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.fillScreen(TFT_BLACK);
    int burn_y = get_burn_shift_y();

    /* Hybrid top zone: Living companion eyes reacting to weather */
    Expression wExpr = EXPR_IDLE;
    if (weather.valid && weather.weather_code >= 95) {
        wExpr = EXPR_SHOCK;
    }
    drawMiniFace(wExpr, g_blinkEyeHeight, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);
    cv.drawFastHLine(6, 25 + burn_y, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);

    /* Hybrid bottom zone: Weather Forecast */
    renderWeatherToCanvas(weather, animFrame, burn_y);
    cv.pushSprite(0, 0);
}

void renderNotificationToCanvas(const NotificationInfo& notif, float animFrame, int offsetY) {
    (void)animFrame;
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.setTextSize(1);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);

    /* Mail badge icon */
    int ix = 8;
    int iy = 28 + offsetY;
    cv.drawRect(ix, iy, 9, 7, TFT_WHITE);
    cv.drawLine(ix, iy, ix + 4, iy + 3, TFT_WHITE);
    cv.drawLine(ix + 8, iy, ix + 4, iy + 3, TFT_WHITE);

    char header_buf[24];
    snprintf(header_buf, sizeof(header_buf), "%s", (notif.title[0] != '\0') ? notif.title : notif.app);
    cv.setCursor(ix + 13, iy);
    cv.print(header_buf);

    const char* msg = (notif.message[0] != '\0') ? notif.message : "New alert";
    size_t len = strlen(msg);

    int line_y = 39 + offsetY;
    size_t pos = 0;
    for (int l = 0; l < 2 && pos < len; l++) {
        char line_buf[22];
        size_t take = (len - pos > 19) ? 19 : (len - pos);
        if (take == 19 && pos + take < len && msg[pos + take] != ' ' && msg[pos + take - 1] != ' ') {
            size_t last_space = 0;
            for (size_t s = 0; s < take; s++) {
                if (msg[pos + s] == ' ') last_space = s;
            }
            if (last_space > 5) take = last_space;
        }
        strncpy(line_buf, msg + pos, take);
        line_buf[take] = '\0';
        while (pos + take < len && msg[pos + take] == ' ') take++;
        pos += take;

        cv.setCursor(8, line_y);
        cv.print(line_buf);
        line_y += 10;
    }
}

void drawNotificationScreen(const NotificationInfo& notif, float animFrame) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.fillScreen(TFT_BLACK);
    int burn_y = get_burn_shift_y();

    /* Hybrid top zone: Expressive shocked / alert eyes */
    drawMiniFace(EXPR_SHOCK, g_blinkEyeHeight, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);
    cv.drawFastHLine(6, 25 + burn_y, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);

    /* Hybrid bottom zone: Notification contents */
    renderNotificationToCanvas(notif, animFrame, burn_y);
    cv.pushSprite(0, 0);
}

static void drawNavIcon(NavIconType icon, int cx, int cy, int size) {
    (void)size;
    LGFX_Sprite& cv = *s_ambient_canvas;
    switch (icon) {
        case NAV_ICON_TURN_RIGHT:
            cv.drawFastVLine(cx - 6, cy - 2, 14, TFT_WHITE);
            cv.drawFastVLine(cx - 5, cy - 2, 14, TFT_WHITE);
            cv.drawFastVLine(cx - 4, cy - 2, 14, TFT_WHITE);
            cv.drawFastHLine(cx - 4, cy - 2, 10, TFT_WHITE);
            cv.drawFastHLine(cx - 4, cy - 1, 10, TFT_WHITE);
            cv.drawFastHLine(cx - 4, cy, 10, TFT_WHITE);
            cv.fillTriangle(cx + 12, cy - 1, cx + 5, cy - 8, cx + 5, cy + 6, TFT_WHITE);
            break;

        case NAV_ICON_TURN_LEFT:
            cv.drawFastVLine(cx + 6, cy - 2, 14, TFT_WHITE);
            cv.drawFastVLine(cx + 5, cy - 2, 14, TFT_WHITE);
            cv.drawFastVLine(cx + 4, cy - 2, 14, TFT_WHITE);
            cv.drawFastHLine(cx - 6, cy - 2, 10, TFT_WHITE);
            cv.drawFastHLine(cx - 6, cy - 1, 10, TFT_WHITE);
            cv.drawFastHLine(cx - 6, cy, 10, TFT_WHITE);
            cv.fillTriangle(cx - 12, cy - 1, cx - 5, cy - 8, cx - 5, cy + 6, TFT_WHITE);
            break;

        case NAV_ICON_SLIGHT_RIGHT:
            cv.drawLine(cx - 5, cy + 10, cx + 5, cy - 2, TFT_WHITE);
            cv.drawLine(cx - 4, cy + 10, cx + 6, cy - 2, TFT_WHITE);
            cv.drawLine(cx - 6, cy + 10, cx + 4, cy - 2, TFT_WHITE);
            cv.fillTriangle(cx + 11, cy - 8, cx + 1, cy - 8, cx + 8, cy + 2, TFT_WHITE);
            break;

        case NAV_ICON_SLIGHT_LEFT:
            cv.drawLine(cx + 5, cy + 10, cx - 5, cy - 2, TFT_WHITE);
            cv.drawLine(cx + 4, cy + 10, cx - 6, cy - 2, TFT_WHITE);
            cv.drawLine(cx + 6, cy + 10, cx - 4, cy - 2, TFT_WHITE);
            cv.fillTriangle(cx - 11, cy - 8, cx - 1, cy - 8, cx - 8, cy + 2, TFT_WHITE);
            break;

        case NAV_ICON_SHARP_RIGHT:
            cv.drawFastVLine(cx - 4, cy, 12, TFT_WHITE);
            cv.drawFastVLine(cx - 3, cy, 12, TFT_WHITE);
            cv.drawLine(cx - 3, cy, cx + 8, cy - 6, TFT_WHITE);
            cv.drawLine(cx - 3, cy + 1, cx + 8, cy - 5, TFT_WHITE);
            cv.fillTriangle(cx + 13, cy - 8, cx + 6, cy - 12, cx + 6, cy - 1, TFT_WHITE);
            break;

        case NAV_ICON_SHARP_LEFT:
            cv.drawFastVLine(cx + 4, cy, 12, TFT_WHITE);
            cv.drawFastVLine(cx + 3, cy, 12, TFT_WHITE);
            cv.drawLine(cx + 3, cy, cx - 8, cy - 6, TFT_WHITE);
            cv.drawLine(cx + 3, cy + 1, cx - 8, cy - 5, TFT_WHITE);
            cv.fillTriangle(cx - 13, cy - 8, cx - 6, cy - 12, cx - 6, cy - 1, TFT_WHITE);
            break;

        case NAV_ICON_UTURN:
            cv.drawFastVLine(cx + 6, cy - 2, 14, TFT_WHITE);
            cv.drawFastVLine(cx + 5, cy - 2, 14, TFT_WHITE);
            cv.drawCircle(cx, cy - 2, 6, TFT_WHITE);
            cv.drawCircle(cx, cy - 2, 5, TFT_WHITE);
            cv.drawFastVLine(cx - 6, cy - 2, 10, TFT_WHITE);
            cv.drawFastVLine(cx - 5, cy - 2, 10, TFT_WHITE);
            cv.fillTriangle(cx - 5, cy + 12, cx - 10, cy + 6, cx, cy + 6, TFT_WHITE);
            break;

        case NAV_ICON_ROUNDABOUT:
            cv.drawCircle(cx, cy, 7, TFT_WHITE);
            cv.drawCircle(cx, cy, 8, TFT_WHITE);
            cv.drawFastHLine(cx + 6, cy - 3, 6, TFT_WHITE);
            cv.fillTriangle(cx + 14, cy - 3, cx + 8, cy - 8, cx + 8, cy + 2, TFT_WHITE);
            break;

        case NAV_ICON_ARRIVE:
            cv.fillCircle(cx, cy - 4, 6, TFT_WHITE);
            cv.fillCircle(cx, cy - 4, 3, TFT_BLACK);
            cv.fillTriangle(cx, cy + 8, cx - 5, cy - 2, cx + 5, cy - 2, TFT_WHITE);
            break;

        case NAV_ICON_STRAIGHT:
        default:
            cv.drawFastVLine(cx, cy - 2, 14, TFT_WHITE);
            cv.drawFastVLine(cx - 1, cy - 2, 14, TFT_WHITE);
            cv.drawFastVLine(cx + 1, cy - 2, 14, TFT_WHITE);
            cv.fillTriangle(cx, cy - 12, cx - 7, cy - 3, cx + 7, cy - 3, TFT_WHITE);
            break;
    }
}

void renderNavigationToCanvas(const NavigationInfo& nav, float animFrame, int offsetY) {
    (void)animFrame;
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.setTextColor(TFT_WHITE, TFT_BLACK);

    int ix = 4;
    int iy = 2 + offsetY;
    cv.fillRect(ix, iy, 24, 9, TFT_WHITE);
    cv.setTextColor(TFT_BLACK, TFT_WHITE);
    cv.setTextSize(1);
    cv.setCursor(ix + 3, iy + 1);
    cv.print("NAV");

    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    if (nav.eta[0] != '\0') {
        cv.setCursor(34, iy + 1);
        cv.printf("ETA: %s", nav.eta);
    } else if (nav.street[0] != '\0' && nav.distance[0] == '\0') {
        cv.setCursor(34, iy + 1);
        char street_hdr[16];
        strncpy(street_hdr, nav.street, sizeof(street_hdr) - 1);
        street_hdr[sizeof(street_hdr) - 1] = '\0';
        cv.print(street_hdr);
    } else {
        cv.setCursor(34, iy + 1);
        cv.print("Live Route");
    }

    cv.drawFastHLine(4, 13 + offsetY, OLED_PANEL_WIDTH_PX - 8, TFT_WHITE);
    drawNavIcon(nav.icon, 18, 38 + offsetY, 28);

    if (nav.distance[0] != '\0') {
        cv.setTextSize(2);
        cv.setCursor(40, 16 + offsetY);
        cv.print(nav.distance);

        cv.setTextSize(1);
        if (nav.instruction[0] != '\0') {
            cv.setCursor(40, 34 + offsetY);
            char inst_buf[18];
            strncpy(inst_buf, nav.instruction, sizeof(inst_buf) - 1);
            inst_buf[sizeof(inst_buf) - 1] = '\0';
            cv.print(inst_buf);
        }

        cv.setCursor(40, 48 + offsetY);
        if (nav.duration[0] != '\0' && nav.total_dist[0] != '\0') {
            char summary_buf[20];
            snprintf(summary_buf, sizeof(summary_buf), "%s - %s", nav.duration, nav.total_dist);
            cv.print(summary_buf);
        } else if (nav.duration[0] != '\0') {
            char dur_buf[20];
            if (nav.eta[0] != '\0') {
                snprintf(dur_buf, sizeof(dur_buf), "%s (%s)", nav.duration, nav.eta);
            } else {
                snprintf(dur_buf, sizeof(dur_buf), "%s", nav.duration);
            }
            cv.print(dur_buf);
        } else if (nav.total_dist[0] != '\0') {
            char tot_buf[20];
            snprintf(tot_buf, sizeof(tot_buf), "Rem: %s", nav.total_dist);
            cv.print(tot_buf);
        } else if (nav.street[0] != '\0') {
            char street_buf[20];
            strncpy(street_buf, nav.street, sizeof(street_buf) - 1);
            street_buf[sizeof(street_buf) - 1] = '\0';
            cv.print(street_buf);
        }
    } else {
        cv.setTextSize(1);
        if (nav.instruction[0] != '\0') {
            cv.setCursor(40, 18 + offsetY);
            char inst_buf[20];
            strncpy(inst_buf, nav.instruction, sizeof(inst_buf) - 1);
            inst_buf[sizeof(inst_buf) - 1] = '\0';
            cv.print(inst_buf);
        }

        cv.setCursor(40, 32 + offsetY);
        if (nav.duration[0] != '\0' && nav.total_dist[0] != '\0') {
            char summary_buf[20];
            snprintf(summary_buf, sizeof(summary_buf), "%s - %s", nav.duration, nav.total_dist);
            cv.print(summary_buf);
        } else if (nav.duration[0] != '\0') {
            char dur_buf[20];
            snprintf(dur_buf, sizeof(dur_buf), "%s", nav.duration);
            cv.print(dur_buf);
        } else if (nav.total_dist[0] != '\0') {
            char tot_buf[20];
            snprintf(tot_buf, sizeof(tot_buf), "Rem: %s", nav.total_dist);
            cv.print(tot_buf);
        }

        if (nav.street[0] != '\0') {
            cv.setCursor(40, 48 + offsetY);
            char street_buf[20];
            strncpy(street_buf, nav.street, sizeof(street_buf) - 1);
            street_buf[sizeof(street_buf) - 1] = '\0';
            cv.print(street_buf);
        }
    }
}

void drawNavigationScreen(const NavigationInfo& nav, float animFrame) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.fillScreen(TFT_BLACK);
    renderNavigationToCanvas(nav, animFrame, get_burn_shift_y());
    cv.pushSprite(0, 0);
}

void transitionToAmbient(AmbientScreenMode toMode, float durationMs) {
    if (toMode == AMBIENT_NONE) return;
    LGFX_Sprite& cv = *s_ambient_canvas;

    int steps = 11;
    float stepDelay = durationMs / (float)steps;
    Expression startExpr = g_currentExpr;

    WeatherInfo local_weather;
    if (toMode == AMBIENT_WEATHER) {
        portENTER_CRITICAL(&g_weather_mutex);
        local_weather = g_weather_info;
        portEXIT_CRITICAL(&g_weather_mutex);
    }

    NotificationInfo local_notif;
    if (toMode == AMBIENT_NOTIFICATION) {
        portENTER_CRITICAL(&g_notification_mutex);
        local_notif = g_notification_info;
        portEXIT_CRITICAL(&g_notification_mutex);
    }

    NavigationInfo local_nav;
    if (toMode == AMBIENT_NAVIGATION) {
        portENTER_CRITICAL(&g_nav_mutex);
        local_nav = g_nav_info;
        portEXIT_CRITICAL(&g_nav_mutex);
    }

    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;

        if (t <= 0.45f) {
            float p = t / 0.45f;
            float eyeH = fmaxf(0.04f, 1.0f - p * p);
            float squashX = 1.0f + 0.16f * p;
            float squashY = fmaxf(0.04f, 1.0f - 0.96f * p);
            g_currentEyeScaleX = squashX;
            g_currentEyeScaleY = squashY;

            drawFace(startExpr, eyeH, g_currentOffsetX, g_currentOffsetY, 0.0f, g_currentVergence, 1.0f);
        } else {
            float p = (t - 0.45f) / 0.55f;
            float bounceT = eval_elastic_bounce_ease(p);
            int offsetY = (int)roundf((1.0f - bounceT) * 38.0f);

            cv.fillScreen(TFT_BLACK);
            Expression miniExpr = (toMode == AMBIENT_NOTIFICATION) ? EXPR_SHOCK : startExpr;
            drawMiniFace(miniExpr, 1.0f, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);
            cv.drawFastHLine(6, 25 + offsetY, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);

            if (toMode == AMBIENT_CLOCK) {
                renderClockToCanvas(g_animFrame, offsetY);
            } else if (toMode == AMBIENT_WEATHER) {
                renderWeatherToCanvas(local_weather, g_animFrame, offsetY);
            } else if (toMode == AMBIENT_NOTIFICATION) {
                renderNotificationToCanvas(local_notif, g_animFrame, offsetY);
            } else if (toMode == AMBIENT_NAVIGATION) {
                renderNavigationToCanvas(local_nav, g_animFrame, offsetY);
            }
            cv.pushSprite(0, 0);
        }

        vTaskDelay(pdMS_TO_TICKS((int)stepDelay));
    }
    g_currentEyeScaleX = 1.0f;
    g_currentEyeScaleY = 1.0f;
}

void transitionFromAmbientToFace(AmbientScreenMode fromMode, Expression toExpr, float durationMs) {
    if (fromMode == AMBIENT_NONE) return;
    LGFX_Sprite& cv = *s_ambient_canvas;

    int steps = 11;
    float stepDelay = durationMs / (float)steps;

    WeatherInfo local_weather;
    if (fromMode == AMBIENT_WEATHER) {
        portENTER_CRITICAL(&g_weather_mutex);
        local_weather = g_weather_info;
        portEXIT_CRITICAL(&g_weather_mutex);
    }

    NotificationInfo local_notif;
    if (fromMode == AMBIENT_NOTIFICATION) {
        portENTER_CRITICAL(&g_notification_mutex);
        local_notif = g_notification_info;
        portEXIT_CRITICAL(&g_notification_mutex);
    }

    NavigationInfo local_nav;
    if (fromMode == AMBIENT_NAVIGATION) {
        portENTER_CRITICAL(&g_nav_mutex);
        local_nav = g_nav_info;
        portEXIT_CRITICAL(&g_nav_mutex);
    }

    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;

        if (t <= 0.40f) {
            float p = t / 0.40f;
            int offsetY = (int)roundf((p * p) * 38.0f);

            cv.fillScreen(TFT_BLACK);
            drawMiniFace(toExpr, 1.0f, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);
            cv.drawFastHLine(6, 25 + offsetY, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);

            if (fromMode == AMBIENT_CLOCK) {
                renderClockToCanvas(g_animFrame, offsetY);
            } else if (fromMode == AMBIENT_WEATHER) {
                renderWeatherToCanvas(local_weather, g_animFrame, offsetY);
            } else if (fromMode == AMBIENT_NOTIFICATION) {
                renderNotificationToCanvas(local_notif, g_animFrame, offsetY);
            } else if (fromMode == AMBIENT_NAVIGATION) {
                renderNavigationToCanvas(local_nav, g_animFrame, offsetY);
            }
            cv.pushSprite(0, 0);
        } else {
            float p = (t - 0.40f) / 0.60f;
            float eyeH = fmaxf(0.04f, blinkOpenEase(p));
            float squashX = 1.0f, squashY = 1.0f;
            compute_squash_stretch_factors(p, getEmotionArousal(), &squashX, &squashY);
            g_currentEyeScaleX = squashX;
            g_currentEyeScaleY = squashY;

            drawFace(toExpr, eyeH, g_currentOffsetX, g_currentOffsetY, 0.0f, g_currentVergence, 1.0f);
        }

        vTaskDelay(pdMS_TO_TICKS((int)stepDelay));
    }
    g_currentEyeScaleX = 1.0f;
    g_currentEyeScaleY = 1.0f;
}
