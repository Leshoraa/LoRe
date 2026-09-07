/**
 * @file http_server.cpp
 * @brief Embedded asynchronous HTTP web server, JSON telemetry, and REST control implementation.
 */

#include "src/net/http_server.h"
#include "src/net/web_ui.h"
#include "src/net/net_utils.h"
#include "src/net/telemetry_service.h"
#include "src/net/wifi_manager.h"
#include "src/net/weather_client.h"
#include "src/net/notification_client.h"
#include "src/net/ble_manager.h"
#include "src/core/display_engine.h"
#include "src/core/gaze_engine.h"
#include "src/config/device_config.h"
#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include "src/math/affective_engine.h"
#include "src/ai/brain_engine.h"
#include "src/ai/personality_engine.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <WiFi.h>
#include <Update.h>
#include <time.h>
#include <sys/time.h>

httpd_handle_t g_web_httpd = NULL;

static esp_err_t index_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    return httpd_resp_send(req, HTML_PAGE, strlen(HTML_PAGE));
}

static esp_err_t telemetry_handler(httpd_req_t *req) {
    char json[1600];
    formatTelemetryJson(json, sizeof(json));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t set_gaze_handler(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing payload");
        return ESP_FAIL;
    }
    buf[ret] = ' ';
    float x = 0.0f, y = 0.0f, dur = 3000.0f;
    char *px = strstr(buf, "\"x\":");
    char *py = strstr(buf, "\"y\":");
    char *pd = strstr(buf, "\"duration_ms\":");
    if (px) x = atof(px + 4);
    if (py) y = atof(py + 4);
    if (pd) dur = atof(pd + 14);
    setVirtualTarget(x, y, dur);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

static esp_err_t get_wifi_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char json[384];
    snprintf(json, sizeof(json),
        "{\"sta_ssid\":\"%s\",\"sta_pass\":\"%s\",\"ap_ssid\":\"%s\",\"ap_pass\":\"%s\",\"ble_name\":\"%s\",\"is_ap\":%s}",
        get_wifi_sta_ssid(), get_wifi_sta_pass(), get_wifi_ap_ssid(), get_wifi_ap_pass(), get_ble_device_name(),
        isWiFiAPMode() ? "true" : "false"
    );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t switch_mode_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    char target_mode[16] = {0};
    extract_json_field(buf, "mode", target_mode, sizeof(target_mode));

    const char* resp = "{\"status\":\"ok\",\"message\":\"Mode switched. ESP rebooting...\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, strlen(resp));

    switch_wifi_mode(target_mode);
    return ESP_OK;
}

static esp_err_t save_wifi_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[512];
    int ret, remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) remaining = sizeof(buf) - 1;

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char new_sta_ssid[64] = {0};
    char new_sta_pass[64] = {0};
    char new_ap_ssid[64]  = {0};
    char new_ap_pass[64]  = {0};
    char new_ble_name[64] = {0};

    extract_json_field(buf, "sta_ssid", new_sta_ssid, sizeof(new_sta_ssid));
    extract_json_field(buf, "sta_pass", new_sta_pass, sizeof(new_sta_pass));
    extract_json_field(buf, "ap_ssid", new_ap_ssid, sizeof(new_ap_ssid));
    extract_json_field(buf, "ap_pass", new_ap_pass, sizeof(new_ap_pass));
    extract_json_field(buf, "ble_name", new_ble_name, sizeof(new_ble_name));

    const char* resp = "{\"status\":\"ok\",\"message\":\"Settings saved. ESP rebooting...\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, strlen(resp));

    save_all_device_config(new_sta_ssid, new_sta_pass, new_ap_ssid, new_ap_pass, new_ble_name);
    return ESP_OK;
}

static esp_err_t set_expression_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[128];
    int remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) remaining = sizeof(buf) - 1;

    int ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char expr_val[16] = {0};
    if (extract_json_field(buf, "expr", expr_val, sizeof(expr_val))) {
        if (strcmp(expr_val, "auto") == 0 || strcmp(expr_val, "-1") == 0) {
            setManualExpression(-1);
        } else {
            int code = atoi(expr_val);
            if (code >= 0 && code <= 7) {
                setManualExpression(code);
            }
        }
    }

    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t scan_wifi_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, false, false, 300);

    char json[1024];
    int pos = 0;
    pos += snprintf(json + pos, sizeof(json) - pos, "{\"networks\":[");

    for (int i = 0; i < n && i < 15 && pos < (int)sizeof(json) - 80; i++) {
        if (i > 0) pos += snprintf(json + pos, sizeof(json) - pos, ",");
        const char* enc = "OPEN";
        switch (WiFi.encryptionType(i)) {
            case WIFI_AUTH_WPA_PSK:       enc = "WPA"; break;
            case WIFI_AUTH_WPA2_PSK:      enc = "WPA2"; break;
            case WIFI_AUTH_WPA_WPA2_PSK:  enc = "WPA/WPA2"; break;
            case WIFI_AUTH_WPA3_PSK:      enc = "WPA3"; break;
            case WIFI_AUTH_WEP:           enc = "WEP"; break;
            default:                       enc = "OTHER"; break;
        }
        pos += snprintf(json + pos, sizeof(json) - pos,
            "{\"ssid\":\"%s\",\"rssi\":%d,\"enc\":\"%s\"}",
            WiFi.SSID(i).c_str(), WiFi.RSSI(i), enc);
    }
    pos += snprintf(json + pos, sizeof(json) - pos, "],\"count\":%d}", n);

    WiFi.scanDelete();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t update_post_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[1024];
    int remaining = req->content_len;
    bool is_first = true;

    if (remaining <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content-Length required");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        int to_read = remaining < (int)sizeof(buf) ? remaining : (int)sizeof(buf);
        int ret = httpd_req_recv(req, buf, to_read);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            Update.abort();
            return ESP_FAIL;
        }

        if (is_first) {
            is_first = false;
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Update.begin failed");
                return ESP_FAIL;
            }
        }

        if (Update.write((uint8_t*)buf, ret) != (size_t)ret) {
            Update.abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Update.write failed");
            return ESP_FAIL;
        }

        remaining -= ret;
    }

    if (Update.end(true)) {
        const char* resp = "{\"status\":\"ok\",\"message\":\"Firmware flashed! Rebooting...\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, resp, strlen(resp));
        schedule_system_restart(1500);
        return ESP_OK;
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Update.end failed");
        return ESP_FAIL;
    }
}

static esp_err_t set_brightness_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[128];
    int remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) remaining = sizeof(buf) - 1;

    int ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char val_str[32] = {0};
    char save_str[32] = {0};
    extract_json_field(buf, "brightness", val_str, sizeof(val_str));
    extract_json_field(buf, "save", save_str, sizeof(save_str));

    int b = atoi(val_str);
    if (b < 0) b = 0;
    if (b > 255) b = 255;

    setOledBrightnessLive((uint8_t)b);
    if (strcmp(save_str, "true") == 0 || strcmp(save_str, "1") == 0) {
        save_oled_brightness((uint8_t)b);
    }

    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t set_auto_brightness_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[128];
    int remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) remaining = sizeof(buf) - 1;

    int ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char val_str[32] = {0};
    extract_json_field(buf, "enabled", val_str, sizeof(val_str));
    if (val_str[0] == '\0') {
        extract_json_field(buf, "auto_brightness", val_str, sizeof(val_str));
    }

    bool en = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
    setAutoBrightnessLive(en);
    save_auto_brightness_enabled(en);

    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t set_weather_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[256];
    int remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) remaining = sizeof(buf) - 1;

    int ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char city[32] = {0};
    char lat_str[32] = {0};
    char lon_str[32] = {0};
    char en_str[32] = {0};
    char tz_str[32] = {0};

    extract_json_field(buf, "city", city, sizeof(city));
    extract_json_field(buf, "lat", lat_str, sizeof(lat_str));
    extract_json_field(buf, "lon", lon_str, sizeof(lon_str));
    extract_json_field(buf, "enabled", en_str, sizeof(en_str));
    extract_json_field(buf, "tz_offset_sec", tz_str, sizeof(tz_str));
    if (tz_str[0] == '\0') {
        extract_json_field(buf, "tz", tz_str, sizeof(tz_str));
    }

    float lat = (lat_str[0] != '\0') ? (float)atof(lat_str) : get_weather_lat();
    float lon = (lon_str[0] != '\0') ? (float)atof(lon_str) : get_weather_lon();
    bool enabled = (en_str[0] == '\0' || strcmp(en_str, "true") == 0 || strcmp(en_str, "1") == 0);

    save_weather_config(city, lat, lon, enabled);
    if (tz_str[0] != '\0') {
        save_timezone_offset_sec((int32_t)atol(tz_str));
    }
    triggerWeatherFetch();

    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t weather_info_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char json[448];

    portENTER_CRITICAL(&g_weather_mutex);
    snprintf(json, sizeof(json),
        "{\"city\":\"%s\",\"lat\":%.4f,\"lon\":%.4f,\"enabled\":%s,\"valid\":%s,"
        "\"tz_offset_sec\":%ld,"
        "\"temp\":%.1f,\"humidity\":%d,\"code\":%d,\"condition\":\"%s\",\"last_sync_s\":%lu}",
        get_weather_city(), get_weather_lat(), get_weather_lon(),
        is_weather_enabled() ? "true" : "false",
        g_weather_info.valid ? "true" : "false",
        (long)get_timezone_offset_sec(),
        g_weather_info.temperature,
        g_weather_info.humidity,
        g_weather_info.weather_code,
        g_weather_info.condition,
        (unsigned long)((millis() - g_weather_info.last_sync_ms) / 1000)
    );
    portEXIT_CRITICAL(&g_weather_mutex);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t trigger_weather_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    triggerWeatherDisplay(WEATHER_POPUP_DURATION_MS);

    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t trigger_clock_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    triggerClockDisplay(WEATHER_POPUP_DURATION_MS);

    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t sync_time_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret > 0) {
        buf[ret] = '\0';
        char* p_epoch = strstr(buf, "epoch=");
        char* p_tz = strstr(buf, "tz=");
        if (p_epoch) {
            time_t epoch = (time_t)strtoull(p_epoch + 6, NULL, 10);
            if (epoch > 1700000000) {
                struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
                settimeofday(&tv, NULL);
            }
        }
        if (p_tz) {
            int32_t tz_sec = (int32_t)strtol(p_tz + 3, NULL, 10);
            save_timezone_offset_sec(tz_sec);
            apply_timezone_config();
        }
    }
    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t system_info_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char json[512];

    uint32_t uptime_s = millis() / 1000;
    int32_t rssi = (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

    snprintf(json, sizeof(json),
        "{\"firmware\":\"1.0.0\",\"compiled\":\"%s %s\",\"chip\":\"%s\",\"cores\":%d,\"cpu_mhz\":%d,"
        "\"heap_free\":%u,\"heap_min\":%u,\"psram_free\":0,\"psram_total\":0,"
        "\"uptime_s\":%lu,\"wifi_rssi\":%d,\"wifi_mode\":\"%s\",\"ip\":\"%s\","
        "\"camera_present\":false,\"camera_ok\":false,\"stream_clients\":0,\"brightness\":%u,\"auto_brightness\":%s}",
        __DATE__, __TIME__,
        ESP.getChipModel(), ESP.getChipCores(), ESP.getCpuFreqMHz(),
        (unsigned)esp_get_free_heap_size(), (unsigned)esp_get_minimum_free_heap_size(),
        (unsigned long)uptime_s, (int)rssi,
        isWiFiAPMode() ? "AP" : "STA",
        isWiFiAPMode() ? WiFi.softAPIP().toString().c_str() : WiFi.localIP().toString().c_str(),
        (unsigned)g_oled_brightness,
        g_auto_brightness_enabled ? "true" : "false"
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t notify_post_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[256];
    int remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) remaining = sizeof(buf) - 1;

    int ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char app[16] = {0};
    char title[36] = {0};
    char message[96] = {0};

    extract_json_field(buf, "app", app, sizeof(app));
    extract_json_field(buf, "sender", title, sizeof(title));
    if (title[0] == '\0') {
        extract_json_field(buf, "title", title, sizeof(title));
    }
    extract_json_field(buf, "message", message, sizeof(message));
    if (message[0] == '\0') {
        extract_json_field(buf, "msg", message, sizeof(message));
    }
    if (message[0] == '\0') {
        extract_json_field(buf, "text", message, sizeof(message));
    }

    pushLocalNotification(app, title, message);

    const char* resp = "{\"status\":\"ok\",\"message\":\"Notification dispatched\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t ntfy_get_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char json[192];
    snprintf(json, sizeof(json),
        "{\"topic\":\"%s\",\"connected\":%s,\"last_msg_ts\":%u}",
        getNtfyTopic(), isNtfyConnected() ? "true" : "false", (unsigned)getNtfyLastMessageTime()
    );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t ntfy_post_handler(httpd_req_t *req) {
    g_last_web_activity_ms = millis();
    char buf[128];
    int remaining = req->content_len;
    if (remaining >= (int)sizeof(buf)) remaining = sizeof(buf) - 1;

    int ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char topic[64] = {0};
    if (extract_json_field(buf, "topic", topic, sizeof(topic))) {
        setNtfyTopic(topic);
    }

    const char* resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, strlen(resp));
}

void startWebServer(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_PORT_WEB_CONTROL;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 27;
    config.stack_size = 8192;
    config.core_id = 0;
    config.lru_purge_enable = true;

    httpd_uri_t index_uri               = { .uri = "/",                   .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t telemetry_uri           = { .uri = "/telemetry",           .method = HTTP_GET,  .handler = telemetry_handler,           .user_ctx = NULL };
    httpd_uri_t set_expression_uri      = { .uri = "/set_expression",      .method = HTTP_POST, .handler = set_expression_handler,        .user_ctx = NULL };
    httpd_uri_t set_gaze_uri            = { .uri = "/set_gaze",            .method = HTTP_POST, .handler = set_gaze_handler,           .user_ctx = NULL };
    httpd_uri_t get_wifi_uri            = { .uri = "/get_wifi",            .method = HTTP_GET,  .handler = get_wifi_handler,            .user_ctx = NULL };
    httpd_uri_t save_wifi_uri           = { .uri = "/save_wifi",           .method = HTTP_POST, .handler = save_wifi_handler,           .user_ctx = NULL };
    httpd_uri_t switch_mode_uri         = { .uri = "/switch_mode",         .method = HTTP_POST, .handler = switch_mode_handler,         .user_ctx = NULL };
    httpd_uri_t captive_1               = { .uri = "/generate_204",        .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t captive_2               = { .uri = "/gen_204",             .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t captive_3               = { .uri = "/hotspot-detect.html", .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t captive_4               = { .uri = "/connecttest.txt",     .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t captive_5               = { .uri = "/ncsi.txt",            .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t captive_6               = { .uri = "/canonical.html",      .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t captive_7               = { .uri = "/success.txt",         .method = HTTP_GET,  .handler = index_handler,               .user_ctx = NULL };
    httpd_uri_t scan_wifi_uri           = { .uri = "/scan_wifi",           .method = HTTP_GET,  .handler = scan_wifi_handler,           .user_ctx = NULL };
    httpd_uri_t system_info_uri         = { .uri = "/system_info",         .method = HTTP_GET,  .handler = system_info_handler,         .user_ctx = NULL };
    httpd_uri_t update_uri              = { .uri = "/update",              .method = HTTP_POST, .handler = update_post_handler,         .user_ctx = NULL };
    httpd_uri_t set_brightness_uri      = { .uri = "/set_brightness",      .method = HTTP_POST, .handler = set_brightness_handler,     .user_ctx = NULL };
    httpd_uri_t set_auto_brightness_uri = { .uri = "/set_auto_brightness", .method = HTTP_POST, .handler = set_auto_brightness_handler,   .user_ctx = NULL };
    httpd_uri_t set_weather_uri         = { .uri = "/set_weather",         .method = HTTP_POST, .handler = set_weather_handler,        .user_ctx = NULL };
    httpd_uri_t weather_info_uri        = { .uri = "/weather_info",        .method = HTTP_GET,  .handler = weather_info_handler,       .user_ctx = NULL };
    httpd_uri_t trigger_weather_uri     = { .uri = "/trigger_weather",     .method = HTTP_POST, .handler = trigger_weather_handler,    .user_ctx = NULL };
    httpd_uri_t trigger_clock_uri       = { .uri = "/trigger_clock",       .method = HTTP_POST, .handler = trigger_clock_handler,      .user_ctx = NULL };
    httpd_uri_t sync_time_uri           = { .uri = "/sync_time",           .method = HTTP_POST, .handler = sync_time_handler,          .user_ctx = NULL };
    httpd_uri_t notify_uri              = { .uri = "/api/notify",          .method = HTTP_POST, .handler = notify_post_handler,        .user_ctx = NULL };
    httpd_uri_t ntfy_get_uri            = { .uri = "/api/ntfy",            .method = HTTP_GET,  .handler = ntfy_get_handler,          .user_ctx = NULL };
    httpd_uri_t ntfy_post_uri           = { .uri = "/api/ntfy",            .method = HTTP_POST, .handler = ntfy_post_handler,         .user_ctx = NULL };

    if (httpd_start(&g_web_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(g_web_httpd, &index_uri);
        httpd_register_uri_handler(g_web_httpd, &telemetry_uri);
        httpd_register_uri_handler(g_web_httpd, &set_expression_uri);
        httpd_register_uri_handler(g_web_httpd, &set_gaze_uri);
        httpd_register_uri_handler(g_web_httpd, &get_wifi_uri);
        httpd_register_uri_handler(g_web_httpd, &save_wifi_uri);
        httpd_register_uri_handler(g_web_httpd, &switch_mode_uri);
        httpd_register_uri_handler(g_web_httpd, &captive_1);
        httpd_register_uri_handler(g_web_httpd, &captive_2);
        httpd_register_uri_handler(g_web_httpd, &captive_3);
        httpd_register_uri_handler(g_web_httpd, &captive_4);
        httpd_register_uri_handler(g_web_httpd, &captive_5);
        httpd_register_uri_handler(g_web_httpd, &captive_6);
        httpd_register_uri_handler(g_web_httpd, &captive_7);
        httpd_register_uri_handler(g_web_httpd, &scan_wifi_uri);
        httpd_register_uri_handler(g_web_httpd, &system_info_uri);
        httpd_register_uri_handler(g_web_httpd, &update_uri);
        httpd_register_uri_handler(g_web_httpd, &set_brightness_uri);
        httpd_register_uri_handler(g_web_httpd, &set_auto_brightness_uri);
        httpd_register_uri_handler(g_web_httpd, &set_weather_uri);
        httpd_register_uri_handler(g_web_httpd, &weather_info_uri);
        httpd_register_uri_handler(g_web_httpd, &trigger_weather_uri);
        httpd_register_uri_handler(g_web_httpd, &trigger_clock_uri);
        httpd_register_uri_handler(g_web_httpd, &sync_time_uri);
        httpd_register_uri_handler(g_web_httpd, &notify_uri);
        httpd_register_uri_handler(g_web_httpd, &ntfy_get_uri);
        httpd_register_uri_handler(g_web_httpd, &ntfy_post_uri);
        LORE_LOG_INF("HTTP", "Web control server registered on port %d", HTTP_PORT_WEB_CONTROL);
    } else {
        LORE_LOG_ERR("HTTP", "Failed to start HTTP web control server");
    }
}
