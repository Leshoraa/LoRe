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
#include "src/net/wifi_manager.h"
#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <ctype.h>

static LGFX_Sprite* s_ambient_canvas = &canvas;

void set_ambient_canvas(LGFX_Sprite* p_canvas) {
    if (p_canvas) {
        s_ambient_canvas = p_canvas;
    }
}

static void draw_wifi_status_icon(LGFX_Sprite& cv, int x, int y, bool is_connected) {
    if (is_connected) {
        cv.fillRect(x,     y + 4, 2, 3, TFT_WHITE);
        cv.fillRect(x + 3, y + 2, 2, 5, TFT_WHITE);
        cv.fillRect(x + 6, y,     2, 7, TFT_WHITE);
        cv.setTextSize(1);
        cv.setTextColor(TFT_WHITE, TFT_BLACK);
        cv.setCursor(x + 10, y);
        cv.print("ON");
    } else {
        cv.drawRect(x,     y + 4, 2, 3, TFT_WHITE);
        cv.drawRect(x + 3, y + 2, 2, 5, TFT_WHITE);
        cv.drawRect(x + 6, y,     2, 7, TFT_WHITE);
        cv.drawLine(x - 1, y - 1, x + 9, y + 8, TFT_WHITE);
        cv.setTextSize(1);
        cv.setTextColor(TFT_WHITE, TFT_BLACK);
        cv.setCursor(x + 10, y);
        cv.print("NC");
    }
}

static void draw_heartbeat_icon(LGFX_Sprite& cv, int x, int y, bool is_beating, bool is_synced) {
    if (is_beating) {
        cv.fillRect(x + 1, y,     2, 2, TFT_WHITE);
        cv.fillRect(x + 5, y,     2, 2, TFT_WHITE);
        cv.fillRect(x,     y + 2, 8, 2, TFT_WHITE);
        cv.fillRect(x + 1, y + 4, 6, 2, TFT_WHITE);
        cv.fillRect(x + 3, y + 6, 2, 1, TFT_WHITE);
    } else {
        cv.drawPixel(x + 1, y,     TFT_WHITE);
        cv.drawPixel(x + 6, y,     TFT_WHITE);
        cv.drawPixel(x,     y + 2, TFT_WHITE);
        cv.drawPixel(x + 7, y + 2, TFT_WHITE);
        cv.drawPixel(x + 2, y + 5, TFT_WHITE);
        cv.drawPixel(x + 5, y + 5, TFT_WHITE);
        cv.drawPixel(x + 3, y + 6, TFT_WHITE);
        cv.drawPixel(x + 4, y + 6, TFT_WHITE);
    }
    cv.setTextSize(1);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.setCursor(x + 10, y);
    cv.print(is_synced ? "OK" : "..");
}

static void draw_weather_glyph(LGFX_Sprite& cv, int weather_code, int icx, int icy, float animFrame) {
    int code = weather_code;
    if (code == 0 || code == 1) {
        /* Sun: solid glowing core + 8 animated rotating corona rays */
        cv.fillCircle(icx, icy, 5, TFT_WHITE);
        for (int i = 0; i < 8; i++) {
            float angle = (float)i * (2.0f * (float)M_PI / 8.0f) + animFrame * 0.16f;
            int x1 = icx + (int)roundf(cosf(angle) * 7.0f);
            int y1 = icy + (int)roundf(sinf(angle) * 7.0f);
            int x2 = icx + (int)roundf(cosf(angle) * 11.0f);
            int y2 = icy + (int)roundf(sinf(angle) * 11.0f);
            cv.drawLine(x1, y1, x2, y2, TFT_WHITE);
        }
    } else if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
        /* Rain: detailed puffy cloud + 3 animated slanted falling raindrops */
        cv.fillCircle(icx - 6, icy - 3, 4, TFT_WHITE);
        cv.fillCircle(icx,     icy - 6, 6, TFT_WHITE);
        cv.fillCircle(icx + 6, icy - 3, 4, TFT_WHITE);
        cv.fillRect(icx - 9,   icy - 3, 18, 5, TFT_WHITE);

        int drop_shift = ((int)(animFrame * 5.0f)) % 5;
        for (int r = -6; r <= 6; r += 6) {
            int rx = icx + r;
            int ry = icy + 3 + drop_shift;
            cv.drawLine(rx, ry, rx - 1, ry + 2, TFT_WHITE);
        }
    } else if (code >= 95 && code <= 99) {
        /* Thunderstorm: cloud + flashing zigzag lightning bolt */
        cv.fillCircle(icx - 6, icy - 3, 4, TFT_WHITE);
        cv.fillCircle(icx,     icy - 6, 6, TFT_WHITE);
        cv.fillCircle(icx + 6, icy - 3, 4, TFT_WHITE);
        cv.fillRect(icx - 9,   icy - 3, 18, 5, TFT_WHITE);

        bool bolt_on = ((int)(animFrame * 6.0f) % 3) != 0;
        if (bolt_on) {
            cv.drawLine(icx,     icy + 2, icx - 3, icy + 6,  TFT_WHITE);
            cv.drawLine(icx - 3, icy + 6, icx + 1, icy + 6,  TFT_WHITE);
            cv.drawLine(icx + 1, icy + 6, icx - 2, icy + 12, TFT_WHITE);
        }
    } else if (code == 45 || code == 48) {
        /* Fog: 3 stratified wavy mist lines */
        cv.drawFastHLine(icx - 11, icy - 5, 22, TFT_WHITE);
        cv.drawFastHLine(icx - 8,  icy,     16, TFT_WHITE);
        cv.drawFastHLine(icx - 11, icy + 5, 22, TFT_WHITE);
    } else {
        /* Clouds / Overcast */
        if (code == 2) {
            /* Partly cloudy: sun disc peeking behind */
            cv.drawCircle(icx + 7, icy - 7, 4, TFT_WHITE);
            cv.fillCircle(icx + 7, icy - 7, 2, TFT_WHITE);
        }
        cv.fillCircle(icx - 6, icy - 3, 5, TFT_WHITE);
        cv.fillCircle(icx,     icy - 6, 7, TFT_WHITE);
        cv.fillCircle(icx + 6, icy - 3, 5, TFT_WHITE);
        cv.fillRect(icx - 9,   icy - 3, 18, 6, TFT_WHITE);
    }
}

static void draw_attention_bell(LGFX_Sprite& cv, int bx, int by, float animFrame) {
    /* Oscillate bell tilting left-right */
    int wiggle = (int)(sinf(animFrame * 7.0f) * 1.6f);
    int cx = bx + wiggle;

    /* Bell dome & flare */
    cv.fillCircle(cx, by - 2, 2, TFT_WHITE);
    cv.fillRect(cx - 3, by - 2, 7, 3, TFT_WHITE);
    cv.drawFastHLine(cx - 4, by + 1, 9, TFT_WHITE);
    /* Bell clapper */
    cv.fillCircle(cx, by + 3, 1, TFT_WHITE);

    /* Dynamic radiating acoustic wave brackets */
    bool pulse = ((int)(animFrame * 4.0f) % 2 == 0);
    if (pulse) {
        cv.drawPixel(cx - 6, by - 2, TFT_WHITE);
        cv.drawPixel(cx - 7, by,     TFT_WHITE);
        cv.drawPixel(cx - 6, by + 2, TFT_WHITE);

        cv.drawPixel(cx + 6, by - 2, TFT_WHITE);
        cv.drawPixel(cx + 7, by,     TFT_WHITE);
        cv.drawPixel(cx + 6, by + 2, TFT_WHITE);
    }
}

static void wrap_notification_text(const char* msg, char lines[2][22]) {
    lines[0][0] = '\0';
    lines[1][0] = '\0';
    if (!msg || msg[0] == '\0') return;

    size_t len = strlen(msg);
    size_t pos = 0;
    for (size_t l = 0; l < 2 && pos < len; l++) {
        size_t take = (len - pos > 20) ? 20 : (len - pos);
        if (take == 20 && pos + take < len && msg[pos + take] != ' ' && msg[pos + take - 1] != ' ') {
            size_t last_space = 0;
            for (size_t s = 0; s < take; s++) {
                if (msg[pos + s] == ' ') last_space = s;
            }
            if (last_space > 5) take = last_space;
        }
        size_t copy_len = (take < 21) ? take : 21;
        strncpy(lines[l], msg + pos, copy_len);
        lines[l][copy_len] = '\0';
        while (pos + take < len && msg[pos + take] == ' ') take++;
        pos += take;
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

    /* Top Header: Wi-Fi status on left */
    draw_wifi_status_icon(cv, 6, 7 + offsetY, isWiFiConnected());

    /* Top Header: Live heartbeat / NTP sync indicator on right */
    bool heart_beat = time_synced ? (timeinfo.tm_sec % 2 == 0) : ((int)(animFrame * 3.0f) % 2 == 0);
    draw_heartbeat_icon(cv, 97, 7 + offsetY, heart_beat, time_synced);

    /* Separator line framing status header */
    cv.drawFastHLine(6, 23 + offsetY, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);
    cv.drawPixel(6, 22 + offsetY, TFT_WHITE);
    cv.drawPixel(OLED_PANEL_WIDTH_PX - 7, 22 + offsetY, TFT_WHITE);

    /* Hero Digital Clock format */
    char hm_buf[8];
    bool colon_on = time_synced ? (timeinfo.tm_sec % 2 == 0) : ((int)(animFrame * 2.5f) % 2 == 0);
    if (time_synced) {
        snprintf(hm_buf, sizeof(hm_buf), "%02d%c%02d", timeinfo.tm_hour, colon_on ? ':' : ' ', timeinfo.tm_min);
    } else {
        snprintf(hm_buf, sizeof(hm_buf), "--%c--", colon_on ? ':' : ' ');
    }

    /* Left analog micro-clock glyph */
    cv.drawCircle(16, 33 + offsetY, 6, TFT_WHITE);
    cv.drawLine(16, 33 + offsetY, 16, 30 + offsetY, TFT_WHITE);
    cv.drawLine(16, 33 + offsetY, 19, 33 + offsetY, TFT_WHITE);

    /* Centered big digital digits */
    cv.setTextSize(2);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.setCursor(30, 25 + offsetY);
    cv.print(hm_buf);

    /* Seconds subscript on the right of the clock */
    cv.setTextSize(1);
    if (time_synced) {
        char sec_buf[4];
        snprintf(sec_buf, sizeof(sec_buf), "%02d", timeinfo.tm_sec);
        cv.setCursor(94, 25 + offsetY);
        cv.print(sec_buf);
        cv.setCursor(94, 33 + offsetY);
        cv.print("s");
    } else {
        cv.setCursor(94, 28 + offsetY);
        cv.print("--");
    }

    /* Date line with Day-of-Week Pill */
    if (time_synced) {
        static const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        static const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

        cv.fillRoundRect(12, 43 + offsetY, 24, 9, 2, TFT_WHITE);
        cv.setTextColor(TFT_BLACK, TFT_WHITE);
        cv.setTextSize(1);
        cv.setCursor(14, 44 + offsetY);
        cv.print(days[timeinfo.tm_wday % 7]);

        cv.setTextColor(TFT_WHITE, TFT_BLACK);
        char date_buf[20];
        snprintf(date_buf, sizeof(date_buf), "%02d %s %04d", timeinfo.tm_mday, months[timeinfo.tm_mon % 12], timeinfo.tm_year + 1900);
        cv.setCursor(42, 44 + offsetY);
        cv.print(date_buf);
    } else {
        cv.setTextSize(1);
        cv.setTextColor(TFT_WHITE, TFT_BLACK);
        cv.setCursor(24, 44 + offsetY);
        cv.print("Synchronizing NTP...");
    }

    /* Dynamic 60-second progress bar track */
    cv.drawFastHLine(14, 56 + offsetY, 100, TFT_WHITE);
    int sec_w = time_synced ? ((timeinfo.tm_sec * 100) / 59) : (((int)(animFrame * 25.0f)) % 100);
    if (sec_w > 100) sec_w = 100;
    if (sec_w > 0) {
        cv.drawFastHLine(14, 55 + offsetY, sec_w, TFT_WHITE);
        cv.drawFastHLine(14, 57 + offsetY, sec_w, TFT_WHITE);
    }
    int head_x = 14 + sec_w;
    if (head_x > 114) head_x = 114;
    cv.fillCircle(head_x, 56 + offsetY, 2, TFT_WHITE);
}

void drawClockScreen(float animFrame) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.fillScreen(TFT_BLACK);
    int burn_y = get_burn_shift_y();

    /* Hybrid top zone: Living companion eyes */
    drawMiniFace(EXPR_IDLE, g_blinkEyeHeight, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);

    /* Hybrid bottom zone: Digital Clock */
    renderClockToCanvas(animFrame, burn_y);
    cv.pushSprite(0, 0);
}

void renderWeatherToCanvas(const WeatherInfo& weather, float animFrame, int offsetY) {
    LGFX_Sprite& cv = *s_ambient_canvas;

    /* Top Header: Left city badge */
    char city_tag[6];
    if (weather.valid && weather.city[0] != '\0') {
        city_tag[0] = (char)toupper((unsigned char)weather.city[0]);
        city_tag[1] = (char)toupper((unsigned char)weather.city[1]);
        city_tag[2] = (char)toupper((unsigned char)weather.city[2]);
        city_tag[3] = '\0';
    } else {
        strncpy(city_tag, "LOC", sizeof(city_tag));
    }
    cv.fillRoundRect(6, 6 + offsetY, 26, 11, 2, TFT_WHITE);
    cv.setTextColor(TFT_BLACK, TFT_WHITE);
    cv.setTextSize(1);
    cv.setCursor(9, 8 + offsetY);
    cv.print(city_tag);

    /* Top Header: Right weather condition tag */
    const char* cond_tag = "NORM";
    if (weather.valid) {
        int c = weather.weather_code;
        if (c == 0 || c == 1) cond_tag = "SUN";
        else if (c == 2 || c == 3) cond_tag = "CLD";
        else if (c == 45 || c == 48) cond_tag = "FOG";
        else if (c >= 51 && c <= 67) cond_tag = "RAIN";
        else if (c >= 71 && c <= 77) cond_tag = "SNOW";
        else if (c >= 80 && c <= 82) cond_tag = "SHWR";
        else if (c >= 95) cond_tag = "STRM";
    }
    cv.drawRoundRect(96, 6 + offsetY, 26, 11, 2, TFT_WHITE);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.setTextSize(1);
    cv.setCursor(100, 8 + offsetY);
    cv.print(cond_tag);

    /* Separator line framing status header */
    cv.drawFastHLine(6, 23 + offsetY, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);
    cv.drawPixel(6, 22 + offsetY, TFT_WHITE);
    cv.drawPixel(OLED_PANEL_WIDTH_PX - 7, 22 + offsetY, TFT_WHITE);

    /* Left Column: Animated weather illustration */
    draw_weather_glyph(cv, weather.weather_code, 20, 42 + offsetY, animFrame);

    /* Vertical divider separating icon from metrics */
    cv.drawFastVLine(38, 26 + offsetY, 34, TFT_WHITE);

    /* Right Column: Big Temperature */
    char temp_buf[12];
    if (weather.valid) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f", weather.temperature);
    } else {
        snprintf(temp_buf, sizeof(temp_buf), "--.-");
    }

    cv.setTextSize(2);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.setCursor(43, 24 + offsetY);
    cv.print(temp_buf);

    int deg_x = 43 + strlen(temp_buf) * 12 + 2;
    cv.drawCircle(deg_x + 2, 25 + offsetY, 2, TFT_WHITE);
    cv.setTextSize(1);
    cv.setCursor(deg_x + 7, 27 + offsetY);
    cv.print("C");

    /* Right Column: City Name */
    cv.setTextSize(1);
    cv.setCursor(43, 42 + offsetY);
    char city_display[16];
    if (weather.valid && weather.city[0] != '\0') {
        strncpy(city_display, weather.city, sizeof(city_display) - 1);
        city_display[sizeof(city_display) - 1] = '\0';
    } else {
        strncpy(city_display, "Weather", sizeof(city_display));
    }
    cv.print(city_display);

    /* Right Column: Dual Metric Capsules (Humidity & Sun Times) */
    cv.drawPixel(45, 53 + offsetY, TFT_WHITE);
    cv.drawLine(44, 54 + offsetY, 46, 54 + offsetY, TFT_WHITE);
    cv.fillRect(43, 55 + offsetY, 5, 3, TFT_WHITE);
    cv.drawPixel(44, 58 + offsetY, TFT_WHITE);
    cv.drawPixel(46, 58 + offsetY, TFT_WHITE);

    cv.setCursor(51, 53 + offsetY);
    if (weather.valid) {
        cv.printf("%d%%", weather.humidity);
    } else {
        cv.print("--%");
    }

    cv.setCursor(84, 53 + offsetY);
    if (weather.valid && weather.sun_times_valid) {
        bool show_sunset = ((int)(animFrame * 0.4f) % 2 == 1);
        if (show_sunset) {
            cv.printf("v%02d:%02d", weather.sunset_hour, weather.sunset_min);
        } else {
            cv.printf("^%02d:%02d", weather.sunrise_hour, weather.sunrise_min);
        }
    } else {
        cv.print("Live");
    }
}

void drawWeatherScreen(const WeatherInfo& weather, float animFrame) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.fillScreen(TFT_BLACK);
    int burn_y = get_burn_shift_y();

    /* Hybrid top zone: Living companion eyes reacting to weather */
    Expression wExpr = EXPR_IDLE;
    if (weather.valid) {
        if (weather.weather_code == 0 || weather.weather_code == 1) {
            wExpr = EXPR_HAPPY;
        }
    }
    drawMiniFace(wExpr, g_blinkEyeHeight, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);

    /* Hybrid bottom zone: Weather Forecast */
    renderWeatherToCanvas(weather, animFrame, burn_y);
    cv.pushSprite(0, 0);
}

void renderNotificationToCanvas(const NotificationInfo& notif, float animFrame, int offsetY, float progress) {
    LGFX_Sprite& cv = *s_ambient_canvas;

    /* Top Header: Inverted solid App badge on left */
    const char* app_label = "ALERT";
    const char* app_src = (notif.app[0] != '\0') ? notif.app : notif.title;
    if (strstr(app_src, "WhatsApp") || strstr(app_src, "WA")) {
        app_label = "WA";
    } else if (strstr(app_src, "Telegram") || strstr(app_src, "TG")) {
        app_label = "TG";
    } else if (strstr(app_src, "Gmail") || strstr(app_src, "Mail")) {
        app_label = "MAIL";
    } else if (strstr(app_src, "SMS") || strstr(app_src, "Pesan")) {
        app_label = "SMS";
    } else if (strstr(app_src, "Discord")) {
        app_label = "DISC";
    }

    cv.fillRoundRect(4, 6 + offsetY, 34, 11, 2, TFT_WHITE);
    cv.setTextColor(TFT_BLACK, TFT_WHITE);
    cv.setTextSize(1);
    cv.setCursor(7, 8 + offsetY);
    cv.print(app_label);

    /* Top Header: Continuous animated ringing bell beacon on right */
    draw_attention_bell(cv, 110, 11 + offsetY, animFrame);

    /* Separator line framing status header */
    cv.drawFastHLine(4, 23 + offsetY, OLED_PANEL_WIDTH_PX - 8, TFT_WHITE);
    cv.drawPixel(4, 22 + offsetY, TFT_WHITE);
    cv.drawPixel(OLED_PANEL_WIDTH_PX - 5, 22 + offsetY, TFT_WHITE);

    /* Message Card: Sender / Title Line */
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.setTextSize(1);
    cv.setCursor(4, 25 + offsetY);

    char sender_buf[22];
    const char* sender = (notif.title[0] != '\0') ? notif.title : notif.app;
    if (sender[0] != '\0') {
        snprintf(sender_buf, sizeof(sender_buf), ">> %s", sender);
    } else {
        snprintf(sender_buf, sizeof(sender_buf), ">> New Message");
    }
    cv.print(sender_buf);

    /* Message Body: Clean word-wrapped lines */
    char lines[2][22];
    wrap_notification_text((notif.message[0] != '\0') ? notif.message : "New alert received", lines);

    cv.setCursor(4, 36 + offsetY);
    cv.print(lines[0]);

    if (lines[1][0] != '\0') {
        cv.setCursor(4, 47 + offsetY);
        cv.print(lines[1]);
    }

    /* Bottom Timeout Countdown Progress Bar */
    int bar_w = (int)(progress * 120.0f);
    if (bar_w < 0) bar_w = 0;
    if (bar_w > 120) bar_w = 120;
    cv.drawRect(4, 59 + offsetY, 120, 3, TFT_WHITE);
    if (bar_w > 2) {
        cv.fillRect(5, 60 + offsetY, bar_w - 2, 1, TFT_WHITE);
    }
}

void drawNotificationScreen(const NotificationInfo& notif, float animFrame, float progress) {
    LGFX_Sprite& cv = *s_ambient_canvas;
    cv.fillScreen(TFT_BLACK);
    int burn_y = get_burn_shift_y();

    /* Hybrid top zone: Expressive cheerful / alert eyes */
    drawMiniFace(EXPR_HAPPY, g_blinkEyeHeight, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);

    /* Notification contents with high-contrast badge, animated bell, and countdown */
    renderNotificationToCanvas(notif, animFrame, burn_y, progress);
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
            Expression miniExpr = (toMode == AMBIENT_NOTIFICATION) ? EXPR_HAPPY : startExpr;
            drawMiniFace(miniExpr, 1.0f, g_currentOffsetX * 0.35f, g_currentOffsetY * 0.35f, 0.5f);
            cv.drawFastHLine(6, 23 + offsetY, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);

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
            cv.drawFastHLine(6, 23 + offsetY, OLED_PANEL_WIDTH_PX - 12, TFT_WHITE);

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
