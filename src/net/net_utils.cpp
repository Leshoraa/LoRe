/**
 * @file net_utils.cpp
 * @brief Network parsing utilities, JSON string extraction, and notification classification implementation.
 */

#include "src/net/net_utils.h"
#include <string.h>
#include <ctype.h>

bool extract_json_field(const char* json, const char* key, char* out, size_t max_len) {
    if (!json || !key || !out || max_len == 0) return false;

    char key_pattern[64];
    snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    const char* p = strstr(json, key_pattern);

    if (p) {
        p += strlen(key_pattern);
        while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
        if (*p == ':') {
            p++;
            while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
            if (*p == '"') {
                p++;
                const char* end = strchr(p, '"');
                if (!end) return false;
                size_t len = (size_t)(end - p);
                if (len >= max_len) len = max_len - 1;
                strncpy(out, p, len);
                out[len] = '\0';
                return true;
            } else {
                size_t i = 0;
                while (*p && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && i < max_len - 1) {
                    out[i++] = *p++;
                }
                out[i] = '\0';
                return (i > 0);
            }
        }
    }

    snprintf(key_pattern, sizeof(key_pattern), "%s=", key);
    p = strstr(json, key_pattern);
    if (p) {
        p += strlen(key_pattern);
        size_t i = 0;
        while (*p && *p != '&' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && i < max_len - 1) {
            out[i++] = *p++;
        }
        out[i] = '\0';
        return (i > 0);
    }

    return false;
}

void classify_notification_app(const char* title, const char* message, char* out_app, size_t max_len) {
    if (!out_app || max_len == 0) return;

    const char* t = title ? title : "";
    const char* m = message ? message : "";

    if (strstr(t, "WhatsApp") || strstr(t, "[WA]") || strstr(m, "WhatsApp")) {
        strncpy(out_app, "WhatsApp", max_len - 1);
    } else if (strstr(t, "Telegram") || strstr(t, "[TG]") || strstr(m, "Telegram")) {
        strncpy(out_app, "Telegram", max_len - 1);
    } else if (strstr(t, "Gmail") || strstr(t, "Email")) {
        strncpy(out_app, "Gmail", max_len - 1);
    } else if (strstr(t, "SMS") || strstr(t, "Pesan")) {
        strncpy(out_app, "SMS", max_len - 1);
    } else if (strstr(t, "Discord")) {
        strncpy(out_app, "Discord", max_len - 1);
    } else {
        strncpy(out_app, "Notice", max_len - 1);
    }
    out_app[max_len - 1] = '\0';
}

NavIconType parse_nav_icon_type(const char* icon_str) {
    if (!icon_str) return NAV_ICON_STRAIGHT;

    if (strcmp(icon_str, "turn_right") == 0 || strcmp(icon_str, "right") == 0) {
        return NAV_ICON_TURN_RIGHT;
    } else if (strcmp(icon_str, "turn_left") == 0 || strcmp(icon_str, "left") == 0) {
        return NAV_ICON_TURN_LEFT;
    } else if (strcmp(icon_str, "slight_right") == 0) {
        return NAV_ICON_SLIGHT_RIGHT;
    } else if (strcmp(icon_str, "slight_left") == 0) {
        return NAV_ICON_SLIGHT_LEFT;
    } else if (strcmp(icon_str, "sharp_right") == 0) {
        return NAV_ICON_SHARP_RIGHT;
    } else if (strcmp(icon_str, "sharp_left") == 0) {
        return NAV_ICON_SHARP_LEFT;
    } else if (strcmp(icon_str, "u_turn") == 0 || strcmp(icon_str, "uturn") == 0) {
        return NAV_ICON_UTURN;
    } else if (strcmp(icon_str, "roundabout") == 0) {
        return NAV_ICON_ROUNDABOUT;
    } else if (strcmp(icon_str, "arrive") == 0 || strcmp(icon_str, "destination") == 0) {
        return NAV_ICON_ARRIVE;
    }

    return NAV_ICON_STRAIGHT;
}
