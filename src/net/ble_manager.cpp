/**
 * @file ble_manager.cpp
 * @brief Bluetooth Low Energy (BLE) Nordic UART Service (NUS) GATT server implementation.
 */

#include "src/net/ble_manager.h"
#include "src/net/net_utils.h"
#include "src/net/telemetry_service.h"
#include "src/net/notification_client.h"
#include "src/core/display_engine.h"
#include "src/net/wifi_manager.h"
#include "src/net/weather_client.h"
#include "src/config/device_config.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include "src/ai/brain_engine.h"
#include "src/ai/personality_engine.h"
#include "src/math/affective_engine.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_bt.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <time.h>
#include <sys/time.h>

static BLEServer *s_pServer = nullptr;
static BLECharacteristic *s_pRxCharacteristic = nullptr;
static BLECharacteristic *s_pTxCharacteristic = nullptr;
static volatile bool s_device_connected = false;
static volatile bool s_ble_telemetry_streaming = false;
static volatile uint32_t s_ble_telemetry_interval_ms = 500;
static TaskHandle_t s_ble_telemetry_task_handle = NULL;

static SemaphoreHandle_t s_ble_tx_mutex = NULL;
static char s_ble_telemetry_buf[1400];

static void sendBleData(const char* data, size_t len) {
    if (!s_pTxCharacteristic || !s_device_connected || len == 0) return;
    if (s_ble_tx_mutex && xSemaphoreTakeRecursive(s_ble_tx_mutex, pdMS_TO_TICKS(150)) != pdTRUE) {
        return;
    }

    const size_t CHUNK_SIZE = 120;
    size_t offset = 0;
    while (offset < len && s_device_connected) {
        size_t to_send = len - offset;
        if (to_send > CHUNK_SIZE) to_send = CHUNK_SIZE;
        s_pTxCharacteristic->setValue((uint8_t*)(data + offset), to_send);
        s_pTxCharacteristic->notify();
        offset += to_send;
        if (offset < len) {
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }

    if (s_ble_tx_mutex) {
        xSemaphoreGiveRecursive(s_ble_tx_mutex);
    }
}

void sendBleTelemetryNow(void) {
    if (!s_pTxCharacteristic || !s_device_connected) return;
    if (s_ble_tx_mutex && xSemaphoreTakeRecursive(s_ble_tx_mutex, pdMS_TO_TICKS(150)) != pdTRUE) {
        return;
    }

    formatTelemetryJson(s_ble_telemetry_buf, sizeof(s_ble_telemetry_buf));
    sendBleData(s_ble_telemetry_buf, strlen(s_ble_telemetry_buf));

    if (s_ble_tx_mutex) {
        xSemaphoreGiveRecursive(s_ble_tx_mutex);
    }
}

void setBleTelemetryStreaming(bool enable, uint32_t interval_ms) {
    s_ble_telemetry_streaming = enable;
    if (interval_ms >= 100) {
        s_ble_telemetry_interval_ms = interval_ms;
    }
    LORE_LOG_INF("BLE", "BLE telemetry streaming %s (interval=%ums)",
                 enable ? "ENABLED" : "DISABLED", (unsigned)s_ble_telemetry_interval_ms);
}

bool isBleTelemetryStreaming(void) {
    return s_ble_telemetry_streaming && s_device_connected;
}

static void bleTelemetryStreamTask(void *pvParameters) {
    (void)pvParameters;
    while (true) {
        if (s_device_connected && s_ble_telemetry_streaming) {
            sendBleTelemetryNow();
            uint32_t wait_time = s_ble_telemetry_interval_ms;
            if (wait_time < 100) wait_time = 100;
            vTaskDelay(pdMS_TO_TICKS(wait_time));
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

static void processIncomingBleData(const String& raw_input) {
    String input = raw_input;
    input.trim();
    if (input.length() == 0) return;

    if (input.startsWith("{") && input.endsWith("}")) {
        char cmd[32] = {0};
        char bright_str[32] = {0};
        char save_str[16] = {0};

        extract_json_field(input.c_str(), "cmd", cmd, sizeof(cmd));
        if (cmd[0] == '\0') {
            extract_json_field(input.c_str(), "type", cmd, sizeof(cmd));
        }

        bool is_brightness_cmd = (strcmp(cmd, "set_brightness") == 0 || strcmp(cmd, "brightness") == 0);
        bool has_brightness_field = extract_json_field(input.c_str(), "brightness", bright_str, sizeof(bright_str));
        if (!has_brightness_field) {
            has_brightness_field = extract_json_field(input.c_str(), "val", bright_str, sizeof(bright_str));
        }
        if (!has_brightness_field) {
            has_brightness_field = extract_json_field(input.c_str(), "value", bright_str, sizeof(bright_str));
        }

        if (is_brightness_cmd || (has_brightness_field && cmd[0] != '\0' && strcmp(cmd, "notification") != 0)) {
            if (bright_str[0] != '\0') {
                int b = atoi(bright_str);
                b = constrain(b, 0, 255);
                setOledBrightnessLive((uint8_t)b);

                extract_json_field(input.c_str(), "save", save_str, sizeof(save_str));
                bool should_save = (save_str[0] == '\0') || (strcmp(save_str, "true") == 0) || (strcmp(save_str, "1") == 0);
                if (should_save) {
                    save_oled_brightness((uint8_t)b);
                }

                LORE_LOG_INF("BLE", "OLED brightness updated via BLE: %d (save=%d)", b, should_save ? 1 : 0);

                if (s_pTxCharacteristic && s_device_connected) {
                    char resp[64];
                    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"brightness\":%d}", b);
                    sendBleData(resp, strlen(resp));
                }
                return;
            }
        }

        char auto_bright_str[16] = {0};
        bool has_auto_bright_field = extract_json_field(input.c_str(), "auto_brightness", auto_bright_str, sizeof(auto_bright_str));
        if (!has_auto_bright_field) {
            has_auto_bright_field = extract_json_field(input.c_str(), "auto_sync_brightness", auto_bright_str, sizeof(auto_bright_str));
        }
        if (strcmp(cmd, "set_auto_brightness") == 0 || (has_auto_bright_field && cmd[0] != '\0')) {
            bool en = (strcmp(auto_bright_str, "true") == 0 || strcmp(auto_bright_str, "1") == 0);
            setAutoBrightnessLive(en);
            save_auto_brightness_enabled(en);
            LORE_LOG_INF("BLE", "Smart Auto Brightness set via BLE: %d", en ? 1 : 0);
            if (s_pTxCharacteristic && s_device_connected) {
                char resp[64];
                snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"auto_brightness\":%s}", en ? "true" : "false");
                sendBleData(resp, strlen(resp));
            }
            return;
        }

        if (strcmp(cmd, "nav") == 0 || strcmp(cmd, "navigation") == 0) {
            char active_str[16] = {0};
            extract_json_field(input.c_str(), "active", active_str, sizeof(active_str));
            bool is_active = (active_str[0] == '\0') || (strcmp(active_str, "true") == 0) || (strcmp(active_str, "1") == 0);

            if (!is_active) {
                LORE_LOG_INF("BLE", "Navigation deactivated via BLE");
                dismissNavigationDisplay();
                return;
            }

            char icon_str[32] = {0};
            char dist_str[16] = {0};
            char inst_str[32] = {0};
            char street_str[48] = {0};
            char eta_str[16] = {0};
            char dur_str[16] = {0};
            char tot_dist_str[16] = {0};

            extract_json_field(input.c_str(), "icon", icon_str, sizeof(icon_str));
            extract_json_field(input.c_str(), "dist", dist_str, sizeof(dist_str));
            if (dist_str[0] == '\0') extract_json_field(input.c_str(), "distance", dist_str, sizeof(dist_str));
            extract_json_field(input.c_str(), "inst", inst_str, sizeof(inst_str));
            if (inst_str[0] == '\0') extract_json_field(input.c_str(), "instruction", inst_str, sizeof(inst_str));
            extract_json_field(input.c_str(), "street", street_str, sizeof(street_str));
            extract_json_field(input.c_str(), "eta", eta_str, sizeof(eta_str));
            extract_json_field(input.c_str(), "dur", dur_str, sizeof(dur_str));
            if (dur_str[0] == '\0') extract_json_field(input.c_str(), "duration", dur_str, sizeof(dur_str));
            extract_json_field(input.c_str(), "tot_dist", tot_dist_str, sizeof(tot_dist_str));
            if (tot_dist_str[0] == '\0') extract_json_field(input.c_str(), "total_dist", tot_dist_str, sizeof(tot_dist_str));

            NavIconType icon_type = parse_nav_icon_type(icon_str);

            NavigationInfo nav;
            memset(&nav, 0, sizeof(nav));
            nav.icon = icon_type;
            strncpy(nav.distance, dist_str, sizeof(nav.distance) - 1);
            strncpy(nav.instruction, inst_str, sizeof(nav.instruction) - 1);
            strncpy(nav.street, street_str, sizeof(nav.street) - 1);
            strncpy(nav.eta, eta_str, sizeof(nav.eta) - 1);
            strncpy(nav.duration, dur_str, sizeof(nav.duration) - 1);
            strncpy(nav.total_dist, tot_dist_str, sizeof(nav.total_dist) - 1);
            nav.active = true;
            nav.valid = true;
            nav.updated_ms = millis();

            LORE_LOG_INF("BLE", "Navigation HUD updated: [%s] dist=%s inst=%s eta=%s dur=%s tot=%s", icon_str, nav.distance, nav.instruction, nav.eta, nav.duration, nav.total_dist);
            triggerNavigationDisplay(nav, 8000);
            return;
        }

        if (strcmp(cmd, "set_expression") == 0 || strcmp(cmd, "expression") == 0 || strcmp(cmd, "set_expr") == 0) {
            char expr_str[16] = {0};
            bool has_expr = extract_json_field(input.c_str(), "expr", expr_str, sizeof(expr_str));
            if (!has_expr) {
                has_expr = extract_json_field(input.c_str(), "expression", expr_str, sizeof(expr_str));
            }
            if (!has_expr) {
                has_expr = extract_json_field(input.c_str(), "val", expr_str, sizeof(expr_str));
            }

            if (has_expr && expr_str[0] != '\0') {
                if (strcmp(expr_str, "auto") == 0 || strcmp(expr_str, "-1") == 0) {
                    setManualExpression(-1);
                    LORE_LOG_INF("BLE", "Expression reset to autonomous Auto Mood via BLE");
                } else {
                    int code = atoi(expr_str);
                    if (code >= 0 && code < NUM_EXPRESSIONS) {
                        setManualExpression(code);
                        LORE_LOG_INF("BLE", "Expression set to %d (%s) via BLE", code, getExpressionName((Expression)code));
                    }
                }

                if (s_pTxCharacteristic && s_device_connected) {
                    char resp[64];
                    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"expr\":\"%s\"}", expr_str);
                    sendBleData(resp, strlen(resp));
                }
                return;
            }
        }

        if (strcmp(cmd, "get_ip") == 0 || strcmp(cmd, "status") == 0 || strcmp(cmd, "info") == 0) {
            if (s_pTxCharacteristic && s_device_connected) {
                char resp[128];
                String current_ip = isWiFiAPMode() ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
                snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"ip\":\"%s\",\"mode\":\"%s\"}", current_ip.c_str(), isWiFiAPMode() ? "AP" : "STA");
                sendBleData(resp, strlen(resp));
                LORE_LOG_INF("BLE", "Reported Wi-Fi IP %s via BLE", current_ip.c_str());
            }
            return;
        }

        if (strcmp(cmd, "get_config") == 0 || strcmp(cmd, "get_network") == 0 || strcmp(cmd, "get_device_config") == 0) {
            if (s_pTxCharacteristic && s_device_connected) {
                char resp[300];
                String current_ip = isWiFiAPMode() ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
                snprintf(resp, sizeof(resp),
                    "{\"status\":\"ok\",\"sta_ssid\":\"%s\",\"sta_pass\":\"%s\",\"ap_ssid\":\"%s\",\"ap_pass\":\"%s\",\"ble_name\":\"%s\",\"ip\":\"%s\",\"is_ap\":%s}",
                    get_wifi_sta_ssid(), get_wifi_sta_pass(), get_wifi_ap_ssid(), get_wifi_ap_pass(), get_ble_device_name(),
                    current_ip.c_str(), isWiFiAPMode() ? "true" : "false");
                sendBleData(resp, strlen(resp));
                LORE_LOG_INF("BLE", "Reported full device config via BLE");
            }
            return;
        }

        if (strcmp(cmd, "save_device_config") == 0 || strcmp(cmd, "save_config") == 0) {
            char sta_s[64] = {0};
            char sta_p[64] = {0};
            char ap_s[64] = {0};
            char ap_p[64] = {0};
            char ble_n[64] = {0};

            extract_json_field(input.c_str(), "sta_ssid", sta_s, sizeof(sta_s));
            extract_json_field(input.c_str(), "sta_pass", sta_p, sizeof(sta_p));
            extract_json_field(input.c_str(), "ap_ssid", ap_s, sizeof(ap_s));
            extract_json_field(input.c_str(), "ap_pass", ap_p, sizeof(ap_p));
            extract_json_field(input.c_str(), "ble_name", ble_n, sizeof(ble_n));

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[128] = "{\"status\":\"ok\",\"message\":\"Config saved. ESP rebooting...\"}";
                sendBleData(resp, strlen(resp));
            }

            save_all_device_config(sta_s, sta_p, ap_s, ap_p, ble_n);
            return;
        }

        if (strcmp(cmd, "save_wifi") == 0) {
            char sta_s[64] = {0};
            char sta_p[64] = {0};
            char ap_s[64] = {0};
            char ap_p[64] = {0};
            extract_json_field(input.c_str(), "sta_ssid", sta_s, sizeof(sta_s));
            extract_json_field(input.c_str(), "sta_pass", sta_p, sizeof(sta_p));
            extract_json_field(input.c_str(), "ap_ssid", ap_s, sizeof(ap_s));
            extract_json_field(input.c_str(), "ap_pass", ap_p, sizeof(ap_p));

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[128] = "{\"status\":\"ok\",\"message\":\"WiFi saved. ESP rebooting...\"}";
                sendBleData(resp, strlen(resp));
            }

            save_wifi_credentials(sta_s, sta_p, ap_s, ap_p);
            return;
        }

        if (strcmp(cmd, "save_ble") == 0) {
            char ble_n[64] = {0};
            extract_json_field(input.c_str(), "ble_name", ble_n, sizeof(ble_n));
            if (ble_n[0] == '\0') extract_json_field(input.c_str(), "name", ble_n, sizeof(ble_n));

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[128] = "{\"status\":\"ok\",\"message\":\"BLE saved. ESP rebooting...\"}";
                sendBleData(resp, strlen(resp));
            }

            save_ble_config(ble_n);
            return;
        }

        if (strcmp(cmd, "show_clock") == 0 || strcmp(cmd, "clock") == 0) {
            triggerClockDisplay(WEATHER_POPUP_DURATION_MS);
            LORE_LOG_INF("BLE", "Clock display triggered via BLE");

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[64] = "{\"status\":\"ok\",\"message\":\"Clock triggered\"}";
                sendBleData(resp, strlen(resp));
            }
            return;
        }

        if (strcmp(cmd, "show_weather") == 0 || strcmp(cmd, "weather") == 0) {
            triggerWeatherDisplay(WEATHER_POPUP_DURATION_MS);
            LORE_LOG_INF("BLE", "Weather display triggered via BLE");

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[64] = "{\"status\":\"ok\",\"message\":\"Weather triggered\"}";
                sendBleData(resp, strlen(resp));
            }
            return;
        }

        if (strcmp(cmd, "get_weather") == 0 || strcmp(cmd, "get_weather_config") == 0) {
            if (s_pTxCharacteristic && s_device_connected) {
                char resp[300];
                portENTER_CRITICAL(&g_weather_mutex);
                snprintf(resp, sizeof(resp),
                    "{\"status\":\"ok\",\"city\":\"%s\",\"lat\":%.4f,\"lon\":%.4f,\"enabled\":%s,\"valid\":%s,"
                    "\"tz\":%ld,\"temp\":%.1f,\"humidity\":%d,\"code\":%d,\"condition\":\"%s\"}",
                    get_weather_city(), get_weather_lat(), get_weather_lon(),
                    is_weather_enabled() ? "true" : "false",
                    g_weather_info.valid ? "true" : "false",
                    (long)get_timezone_offset_sec(),
                    g_weather_info.temperature,
                    g_weather_info.humidity,
                    g_weather_info.weather_code,
                    g_weather_info.condition
                );
                portEXIT_CRITICAL(&g_weather_mutex);
                sendBleData(resp, strlen(resp));
                LORE_LOG_INF("BLE", "Reported weather config via BLE");
            }
            return;
        }

        if (strcmp(cmd, "save_weather") == 0 || strcmp(cmd, "set_weather") == 0) {
            char city[32] = {0};
            char lat_s[32] = {0};
            char lon_s[32] = {0};
            char en_s[16] = {0};
            char tz_s[32] = {0};

            extract_json_field(input.c_str(), "city", city, sizeof(city));
            extract_json_field(input.c_str(), "lat", lat_s, sizeof(lat_s));
            extract_json_field(input.c_str(), "lon", lon_s, sizeof(lon_s));
            extract_json_field(input.c_str(), "enabled", en_s, sizeof(en_s));
            extract_json_field(input.c_str(), "tz", tz_s, sizeof(tz_s));
            if (tz_s[0] == '\0') extract_json_field(input.c_str(), "tz_offset_sec", tz_s, sizeof(tz_s));

            float lat = (lat_s[0] != '\0') ? (float)atof(lat_s) : get_weather_lat();
            float lon = (lon_s[0] != '\0') ? (float)atof(lon_s) : get_weather_lon();
            bool enabled = (en_s[0] == '\0' || strcmp(en_s, "true") == 0 || strcmp(en_s, "1") == 0);

            save_weather_config(city, lat, lon, enabled);
            if (tz_s[0] != '\0') {
                save_timezone_offset_sec((int32_t)atol(tz_s));
                apply_timezone_config();
            }
            triggerWeatherFetch();

            LORE_LOG_INF("BLE", "Saved weather config via BLE: city=%s, lat=%.4f, lon=%.4f, en=%d, tz=%s", city, lat, lon, enabled ? 1 : 0, tz_s);

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[128] = "{\"status\":\"ok\",\"message\":\"Weather settings updated\"}";
                sendBleData(resp, strlen(resp));
            }
            return;
        }

        if (strcmp(cmd, "push_weather") == 0 || strcmp(cmd, "sync_weather") == 0) {
            char city[32] = {0};
            char temp_s[16] = {0};
            char hum_s[16] = {0};
            char code_s[16] = {0};
            char cond[32] = {0};

            extract_json_field(input.c_str(), "city", city, sizeof(city));
            extract_json_field(input.c_str(), "temp", temp_s, sizeof(temp_s));
            extract_json_field(input.c_str(), "hum", hum_s, sizeof(hum_s));
            extract_json_field(input.c_str(), "code", code_s, sizeof(code_s));
            extract_json_field(input.c_str(), "cond", cond, sizeof(cond));

            float temp = atof(temp_s);
            int hum = atoi(hum_s);
            int code = atoi(code_s);

            portENTER_CRITICAL(&g_weather_mutex);
            g_weather_info.temperature = temp;
            g_weather_info.humidity = hum;
            g_weather_info.weather_code = code;
            if (city[0] != '\0') {
                strncpy(g_weather_info.city, city, sizeof(g_weather_info.city) - 1);
                g_weather_info.city[sizeof(g_weather_info.city) - 1] = '\0';
            }
            if (cond[0] != '\0') {
                strncpy(g_weather_info.condition, cond, sizeof(g_weather_info.condition) - 1);
                g_weather_info.condition[sizeof(g_weather_info.condition) - 1] = '\0';
            }
            g_weather_info.valid = true;
            g_weather_info.last_sync_ms = millis();
            portEXIT_CRITICAL(&g_weather_mutex);

            LORE_LOG_INF("BLE", "Weather pushed directly from phone BLE: %s, %.1fC, %d%%, code=%d (%s)",
                g_weather_info.city, temp, hum, code, g_weather_info.condition);

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[64] = "{\"status\":\"ok\",\"message\":\"Weather updated\"}";
                sendBleData(resp, strlen(resp));
            }
            return;
        }

        if (strcmp(cmd, "sync_time") == 0 || strcmp(cmd, "time") == 0) {
            char epoch_s[32] = {0};
            char tz_s[16] = {0};
            extract_json_field(input.c_str(), "epoch", epoch_s, sizeof(epoch_s));
            extract_json_field(input.c_str(), "tz", tz_s, sizeof(tz_s));

            time_t epoch = (time_t)strtoull(epoch_s, NULL, 10);
            if (epoch > 1700000000) {
                struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                LORE_LOG_INF("BLE", "RTC Time synced from phone over BLE: epoch=%llu", (unsigned long long)epoch);
            }
            if (tz_s[0] != '\0') {
                save_timezone_offset_sec((int32_t)atol(tz_s));
                apply_timezone_config();
            }

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[64] = "{\"status\":\"ok\",\"message\":\"Time synced\"}";
                sendBleData(resp, strlen(resp));
            }
            return;
        }

        if (strcmp(cmd, "get_telemetry") == 0 || strcmp(cmd, "telemetry") == 0 || strcmp(cmd, "get_tele") == 0) {
            LORE_LOG_INF("BLE", "Telemetry snapshot requested via BLE JSON command");
            sendBleTelemetryNow();
            return;
        }

        if (strcmp(cmd, "stream_telemetry") == 0 || strcmp(cmd, "start_telemetry") == 0 || strcmp(cmd, "stop_telemetry") == 0) {
            char en_s[16] = {0};
            char int_s[16] = {0};
            extract_json_field(input.c_str(), "enable", en_s, sizeof(en_s));
            if (en_s[0] == '\0') extract_json_field(input.c_str(), "active", en_s, sizeof(en_s));
            extract_json_field(input.c_str(), "interval", int_s, sizeof(int_s));
            if (int_s[0] == '\0') extract_json_field(input.c_str(), "interval_ms", int_s, sizeof(int_s));

            bool enable = true;
            if (strcmp(cmd, "stop_telemetry") == 0) {
                enable = false;
            } else if (en_s[0] != '\0') {
                enable = (strcmp(en_s, "true") == 0 || strcmp(en_s, "1") == 0);
            }

            uint32_t interval = (int_s[0] != '\0') ? (uint32_t)atoi(int_s) : s_ble_telemetry_interval_ms;
            setBleTelemetryStreaming(enable, interval);

            if (s_pTxCharacteristic && s_device_connected) {
                char resp[96];
                snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"streaming\":%s,\"interval_ms\":%u}",
                         enable ? "true" : "false", (unsigned)s_ble_telemetry_interval_ms);
                sendBleData(resp, strlen(resp));
            }
            return;
        }
    }

    if (input.startsWith("BRIGHTNESS:") || input.startsWith("brightness:") || input.startsWith("SET_BRIGHTNESS:")) {
        int colon = input.indexOf(':');
        int b = input.substring(colon + 1).toInt();
        b = constrain(b, 0, 255);
        setOledBrightnessLive((uint8_t)b);
        save_oled_brightness((uint8_t)b);

        LORE_LOG_INF("BLE", "OLED brightness set via BLE text command: %d", b);

        if (s_pTxCharacteristic && s_device_connected) {
            char resp[64];
            snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"brightness\":%d}", b);
            sendBleData(resp, strlen(resp));
        }
        return;
    }

    if (input.startsWith("AUTO_BRIGHTNESS:") || input.startsWith("auto_brightness:") || input.startsWith("SET_AUTO_BRIGHTNESS:")) {
        int colon = input.indexOf(':');
        String val = input.substring(colon + 1);
        val.trim();
        bool en = (val == "1" || val.equalsIgnoreCase("true") || val.equalsIgnoreCase("on"));
        setAutoBrightnessLive(en);
        save_auto_brightness_enabled(en);
        LORE_LOG_INF("BLE", "Smart Auto Brightness set via BLE text command: %d", en ? 1 : 0);

        if (s_pTxCharacteristic && s_device_connected) {
            char resp[64];
            snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"auto_brightness\":%s}", en ? "true" : "false");
            sendBleData(resp, strlen(resp));
        }
        return;
    }

    if (input.startsWith("EXPR:") || input.startsWith("expr:") || input.startsWith("SET_EXPRESSION:")) {
        int colon = input.indexOf(':');
        String val = input.substring(colon + 1);
        val.trim();
        if (val.equalsIgnoreCase("auto") || val == "-1") {
            setManualExpression(-1);
            LORE_LOG_INF("BLE", "Expression reset to autonomous Auto Mood via text command");
        } else {
            int code = val.toInt();
            if (code >= 0 && code < NUM_EXPRESSIONS) {
                setManualExpression(code);
                LORE_LOG_INF("BLE", "Expression set to %d (%s) via BLE text command", code, getExpressionName((Expression)code));
            }
        }

        if (s_pTxCharacteristic && s_device_connected) {
            char resp[64];
            snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"expr\":\"%s\"}", val.c_str());
            sendBleData(resp, strlen(resp));
        }
        return;
    }

    if (input.equalsIgnoreCase("TELEMETRY") || input.equalsIgnoreCase("GET_TELEMETRY") || input.startsWith("TELEMETRY:")) {
        LORE_LOG_INF("BLE", "Telemetry snapshot requested via BLE text command");
        sendBleTelemetryNow();
        return;
    }

    if (input.startsWith("STREAM_TELEMETRY:") || input.startsWith("stream_telemetry:")) {
        int colon = input.indexOf(':');
        String val = input.substring(colon + 1);
        val.trim();
        bool enable = (val == "1" || val.equalsIgnoreCase("true") || val.equalsIgnoreCase("on"));
        int interval = val.toInt();
        if (interval > 1) {
            setBleTelemetryStreaming(true, (uint32_t)interval);
        } else {
            setBleTelemetryStreaming(enable, s_ble_telemetry_interval_ms);
        }

        if (s_pTxCharacteristic && s_device_connected) {
            char resp[96];
            snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"streaming\":%s,\"interval_ms\":%u}",
                     s_ble_telemetry_streaming ? "true" : "false", (unsigned)s_ble_telemetry_interval_ms);
            sendBleData(resp, strlen(resp));
        }
        return;
    }

    char app[16] = {0};
    char title[36] = {0};
    char message[96] = {0};

    if (input.startsWith("{") && input.endsWith("}")) {
        extract_json_field(input.c_str(), "app", app, sizeof(app));
        extract_json_field(input.c_str(), "title", title, sizeof(title));
        if (title[0] == '\0') {
            extract_json_field(input.c_str(), "sender", title, sizeof(title));
        }
        extract_json_field(input.c_str(), "message", message, sizeof(message));
        if (message[0] == '\0') {
            extract_json_field(input.c_str(), "msg", message, sizeof(message));
        }
        if (message[0] == '\0') {
            extract_json_field(input.c_str(), "text", message, sizeof(message));
        }
    } else if (input.startsWith("[")) {
        int close_bracket = input.indexOf(']');
        if (close_bracket > 1) {
            String app_tag = input.substring(1, close_bracket);
            strncpy(app, app_tag.c_str(), sizeof(app) - 1);
            String rest = input.substring(close_bracket + 1);
            rest.trim();

            int sep_idx = rest.indexOf(':');
            if (sep_idx < 0) sep_idx = rest.indexOf('|');

            if (sep_idx >= 0) {
                String t = rest.substring(0, sep_idx);
                t.trim();
                String m = rest.substring(sep_idx + 1);
                m.trim();
                strncpy(title, t.c_str(), sizeof(title) - 1);
                strncpy(message, m.c_str(), sizeof(message) - 1);
            } else {
                strncpy(title, app_tag.c_str(), sizeof(title) - 1);
                strncpy(message, rest.c_str(), sizeof(message) - 1);
            }
        }
    } else {
        int sep_idx = input.indexOf('|');
        if (sep_idx < 0) sep_idx = input.indexOf(':');

        if (sep_idx >= 0) {
            String t = input.substring(0, sep_idx);
            t.trim();
            String m = input.substring(sep_idx + 1);
            m.trim();
            strncpy(title, t.c_str(), sizeof(title) - 1);
            strncpy(message, m.c_str(), sizeof(message) - 1);
        } else {
            strncpy(title, "Notification", sizeof(title) - 1);
            strncpy(message, input.c_str(), sizeof(message) - 1);
        }
    }

    if (app[0] == '\0') {
        classify_notification_app(title, message, app, sizeof(app));
    }

    if (title[0] == '\0') {
        strncpy(title, "BLE Alert", sizeof(title) - 1);
    }

    if (strlen(message) > 0) {
        LORE_LOG_INF("BLE", "Notification received from BLE [%s] %s: %s", app, title, message);
        pushLocalNotification(app, title, message);
    }
}

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        s_device_connected = true;
        LORE_LOG_INF("BLE", "Phone connected via BLE GATT");
    }

    void onDisconnect(BLEServer* pServer) override {
        s_device_connected = false;
        s_ble_telemetry_streaming = false;
        LORE_LOG_INF("BLE", "Phone disconnected; restarting advertising");
        vTaskDelay(pdMS_TO_TICKS(30));
        BLEDevice::startAdvertising();
    }
};

static QueueHandle_t s_ble_rx_queue = NULL;
static TaskHandle_t s_ble_rx_task_handle = NULL;
static String s_ble_rx_buffer = "";
static unsigned long s_last_rx_ms = 0;

static void bleRxTask(void *pvParameters) {
    (void)pvParameters;
    char *rx_msg = nullptr;
    while (true) {
        if (xQueueReceive(s_ble_rx_queue, &rx_msg, portMAX_DELAY) == pdTRUE && rx_msg) {
            unsigned long now = millis();
            if (s_ble_rx_buffer.length() > 0 && (now - s_last_rx_ms > 1500)) {
                s_ble_rx_buffer = "";
            }
            s_last_rx_ms = now;

            s_ble_rx_buffer += rx_msg;
            free(rx_msg);
            rx_msg = nullptr;

            s_ble_rx_buffer.trim();

            while (true) {
                int start_brace = s_ble_rx_buffer.indexOf('{');
                if (start_brace < 0) {
                    if (s_ble_rx_buffer.length() > 0 && (s_ble_rx_buffer.indexOf('\n') >= 0 || s_ble_rx_buffer.indexOf(':') >= 0 || s_ble_rx_buffer.indexOf('|') >= 0)) {
                        String complete_msg = s_ble_rx_buffer;
                        s_ble_rx_buffer = "";
                        processIncomingBleData(complete_msg);
                    }
                    break;
                }

                int end_brace = s_ble_rx_buffer.indexOf('}', start_brace);
                if (end_brace < 0) {
                    break;
                }

                String single_json = s_ble_rx_buffer.substring(start_brace, end_brace + 1);
                s_ble_rx_buffer = s_ble_rx_buffer.substring(end_brace + 1);
                s_ble_rx_buffer.trim();

                processIncomingBleData(single_json);
            }
        }
    }
}

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        const uint8_t *data = pCharacteristic->getData();
        size_t len = pCharacteristic->getLength();
        if (!data || len == 0 || !s_ble_rx_queue) return;

        char *buf = (char *)malloc(len + 1);
        if (buf) {
            memcpy(buf, data, len);
            buf[len] = '\0';
            if (xQueueSend(s_ble_rx_queue, &buf, 0) != pdTRUE) {
                free(buf);
            }
        }
    }
};

void initBleNotificationServer(void) {
    const char* dev_name = get_ble_device_name();
    LORE_LOG_INF("BLE", "Initializing BLE GATT Server: %s", dev_name);

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    if (!s_ble_tx_mutex) {
        s_ble_tx_mutex = xSemaphoreCreateRecursiveMutex();
    }

    if (!s_ble_rx_queue) {
        s_ble_rx_queue = xQueueCreate(16, sizeof(char*));
    }
    if (!s_ble_rx_task_handle) {
        xTaskCreatePinnedToCore(
            bleRxTask,
            "BLE_Rx_Task",
            6144,
            NULL,
            2,
            &s_ble_rx_task_handle,
            1
        );
    }

    BLEDevice::init(dev_name);
    BLEDevice::setMTU(517);

    BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_DEFAULT);
    BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_ADV);
    BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_SCAN);

    s_pServer = BLEDevice::createServer();
    s_pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = s_pServer->createService(BLE_NUS_SERVICE_UUID);

    s_pTxCharacteristic = pService->createCharacteristic(
        BLE_NUS_CHAR_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    s_pTxCharacteristic->addDescriptor(new BLE2902());

    s_pRxCharacteristic = pService->createCharacteristic(
        BLE_NUS_CHAR_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    s_pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_NUS_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);
    pAdvertising->setMinInterval(32);
    pAdvertising->setMaxInterval(64);
    BLEDevice::startAdvertising();

    if (!s_ble_telemetry_task_handle) {
        xTaskCreatePinnedToCore(
            bleTelemetryStreamTask,
            "BLE_Telem_Task",
            8192,
            NULL,
            1,
            &s_ble_telemetry_task_handle,
            0
        );
    }

    LORE_LOG_INF("BLE", "BLE advertising started as '%s'", dev_name);
}

bool isBleConnected(void) {
    return s_device_connected;
}
