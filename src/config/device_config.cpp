/**
 * @file device_config.cpp
 * @brief Persistent device configuration, NVS storage, and system lifecycle implementation.
 */

#include "src/config/device_config.h"
#include "include/lore_config.h"
#include "include/lore_types.h"
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

static char s_sta_ssid[64] = {0};
static char s_sta_password[64] = {0};
static char s_ap_ssid[64] = {0};
static char s_ap_password[64] = {0};
static char s_ble_name[64] = {0};
static char s_wifi_mode[16] = "AUTO";

static char s_weather_city[32] = WEATHER_DEFAULT_CITY;
static float s_weather_lat = WEATHER_DEFAULT_LAT;
static float s_weather_lon = WEATHER_DEFAULT_LON;
static bool s_is_weather_enabled = true;
static int32_t s_timezone_offset_sec = 7 * 3600;
static uint8_t s_oled_brightness = OLED_DEFAULT_BRIGHTNESS;
static bool s_is_auto_brightness_enabled = DEFAULT_AUTO_BRIGHTNESS;
static bool s_is_oled_deep_sleep_enabled = OLED_DEEP_SLEEP_DEFAULT_ENABLED;

extern volatile bool g_auto_brightness_enabled;

static void restartTask(void *parameters) {
    uint32_t delay_ms = (uint32_t)(uintptr_t)parameters;
    if (delay_ms == 0) {
        delay_ms = 1500;
    }
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    esp_restart();
}

void schedule_system_restart(uint32_t delay_ms) {
    xTaskCreate(restartTask, "restart_task", 2048, (void*)(uintptr_t)delay_ms, 1, NULL);
}

void init_device_config(void) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);

    String stored_mode = prefs.getString("wifi_mode", "AUTO");
    strncpy(s_wifi_mode, stored_mode.c_str(), sizeof(s_wifi_mode) - 1);
    s_wifi_mode[sizeof(s_wifi_mode) - 1] = '\0';

    String stored_sta_ssid = prefs.getString("sta_ssid", prefs.getString("ssid", ""));
    String stored_sta_pass = prefs.getString("sta_pass", prefs.getString("pass", ""));
    String stored_ap_ssid  = prefs.getString("ap_ssid", "");
    String stored_ap_pass  = prefs.getString("ap_pass", "");
    String stored_ble_name = prefs.getString("ble_name", BLE_DEVICE_NAME_DEFAULT);

    if (stored_sta_ssid.length() > 0) {
        strncpy(s_sta_ssid, stored_sta_ssid.c_str(), sizeof(s_sta_ssid) - 1);
        strncpy(s_sta_password, stored_sta_pass.c_str(), sizeof(s_sta_password) - 1);
    } else {
        strncpy(s_sta_ssid, WIFI_STA_DEFAULT_SSID, sizeof(s_sta_ssid) - 1);
        strncpy(s_sta_password, WIFI_STA_DEFAULT_PASS, sizeof(s_sta_password) - 1);
    }
    s_sta_ssid[sizeof(s_sta_ssid) - 1] = '\0';
    s_sta_password[sizeof(s_sta_password) - 1] = '\0';

    if (stored_ap_ssid.length() > 0) {
        strncpy(s_ap_ssid, stored_ap_ssid.c_str(), sizeof(s_ap_ssid) - 1);
        strncpy(s_ap_password, stored_ap_pass.c_str(), sizeof(s_ap_password) - 1);
    } else {
        strncpy(s_ap_ssid, WIFI_FALLBACK_AP_SSID, sizeof(s_ap_ssid) - 1);
        strncpy(s_ap_password, WIFI_FALLBACK_AP_PASS, sizeof(s_ap_password) - 1);
    }
    s_ap_ssid[sizeof(s_ap_ssid) - 1] = '\0';
    s_ap_password[sizeof(s_ap_password) - 1] = '\0';

    if (stored_ble_name.length() > 0) {
        strncpy(s_ble_name, stored_ble_name.c_str(), sizeof(s_ble_name) - 1);
    } else {
        strncpy(s_ble_name, BLE_DEVICE_NAME_DEFAULT, sizeof(s_ble_name) - 1);
    }
    s_ble_name[sizeof(s_ble_name) - 1] = '\0';

    s_oled_brightness = prefs.getUChar("oled_bright", OLED_DEFAULT_BRIGHTNESS);
    g_oled_brightness = s_oled_brightness;

    s_is_auto_brightness_enabled = prefs.getBool("auto_bright", DEFAULT_AUTO_BRIGHTNESS);
    g_auto_brightness_enabled = s_is_auto_brightness_enabled;

    s_is_oled_deep_sleep_enabled = prefs.getBool("oled_sleep", OLED_DEEP_SLEEP_DEFAULT_ENABLED);

    String stored_city = prefs.getString("w_city", WEATHER_DEFAULT_CITY);
    strncpy(s_weather_city, stored_city.c_str(), sizeof(s_weather_city) - 1);
    s_weather_city[sizeof(s_weather_city) - 1] = '\0';

    s_weather_lat = prefs.getFloat("w_lat", WEATHER_DEFAULT_LAT);
    s_weather_lon = prefs.getFloat("w_lon", WEATHER_DEFAULT_LON);
    s_is_weather_enabled = prefs.getBool("w_en", true);
    s_timezone_offset_sec = prefs.getLong("tz_offset", 7 * 3600);

    prefs.end();
}

const char* get_wifi_sta_ssid(void) {
    return s_sta_ssid;
}

const char* get_wifi_sta_pass(void) {
    return s_sta_password;
}

const char* get_wifi_ap_ssid(void) {
    return s_ap_ssid;
}

const char* get_wifi_ap_pass(void) {
    return s_ap_password;
}

const char* get_ble_device_name(void) {
    if (s_ble_name[0] != '\0') {
        return s_ble_name;
    }
    return BLE_DEVICE_NAME_DEFAULT;
}

const char* get_configured_wifi_mode(void) {
    return s_wifi_mode;
}

void apply_timezone_config(void) {
    configTime(s_timezone_offset_sec, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
}

int32_t get_timezone_offset_sec(void) {
    return s_timezone_offset_sec;
}

void save_timezone_offset_sec(int32_t offset_sec) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);
    prefs.putLong("tz_offset", (long)offset_sec);
    prefs.end();
    s_timezone_offset_sec = offset_sec;
    apply_timezone_config();
}

uint8_t get_saved_oled_brightness(void) {
    return s_oled_brightness;
}

void save_oled_brightness(uint8_t brightness) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);
    prefs.putUChar("oled_bright", brightness);
    prefs.end();
    s_oled_brightness = brightness;
    g_oled_brightness = brightness;
}

bool is_auto_brightness_enabled(void) {
    return s_is_auto_brightness_enabled;
}

void save_auto_brightness_enabled(bool is_enabled) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);
    prefs.putBool("auto_bright", is_enabled);
    prefs.end();
    s_is_auto_brightness_enabled = is_enabled;
    g_auto_brightness_enabled = is_enabled;
}

bool is_oled_deep_sleep_enabled(void) {
    return s_is_oled_deep_sleep_enabled;
}

void save_oled_deep_sleep_enabled(bool is_enabled) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);
    prefs.putBool("oled_sleep", is_enabled);
    prefs.end();
    s_is_oled_deep_sleep_enabled = is_enabled;
}

const char* get_weather_city(void) {
    return s_weather_city;
}

float get_weather_lat(void) {
    return s_weather_lat;
}

float get_weather_lon(void) {
    return s_weather_lon;
}

bool is_weather_enabled(void) {
    return s_is_weather_enabled;
}

void save_weather_config(const char* city, float lat, float lon, bool is_enabled) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);
    if (city && city[0] != '\0') {
        prefs.putString("w_city", city);
        strncpy(s_weather_city, city, sizeof(s_weather_city) - 1);
        s_weather_city[sizeof(s_weather_city) - 1] = '\0';
    }
    prefs.putFloat("w_lat", lat);
    prefs.putFloat("w_lon", lon);
    prefs.putBool("w_en", is_enabled);
    prefs.end();

    s_weather_lat = lat;
    s_weather_lon = lon;
    s_is_weather_enabled = is_enabled;
}

void save_ble_config(const char* ble_name) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);
    if (ble_name && ble_name[0] != '\0') {
        prefs.putString("ble_name", ble_name);
        strncpy(s_ble_name, ble_name, sizeof(s_ble_name) - 1);
        s_ble_name[sizeof(s_ble_name) - 1] = '\0';
    }
    prefs.end();
    schedule_system_restart(1500);
}

void save_all_device_config(const char* sta_ssid, const char* sta_pass, const char* ap_ssid, const char* ap_pass, const char* ble_name) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);

    if (sta_ssid && sta_ssid[0] != '\0') {
        prefs.putString("sta_ssid", sta_ssid);
        prefs.putString("ssid", sta_ssid);
        prefs.putString("wifi_mode", "STA");
        strncpy(s_sta_ssid, sta_ssid, sizeof(s_sta_ssid) - 1);
        s_sta_ssid[sizeof(s_sta_ssid) - 1] = '\0';
        strncpy(s_wifi_mode, "STA", sizeof(s_wifi_mode) - 1);
    }
    if (sta_pass && sta_pass[0] != '\0' && strcmp(sta_pass, "********") != 0) {
        prefs.putString("sta_pass", sta_pass);
        prefs.putString("pass", sta_pass);
        strncpy(s_sta_password, sta_pass, sizeof(s_sta_password) - 1);
        s_sta_password[sizeof(s_sta_password) - 1] = '\0';
    }
    if (ap_ssid && ap_ssid[0] != '\0') {
        prefs.putString("ap_ssid", ap_ssid);
        strncpy(s_ap_ssid, ap_ssid, sizeof(s_ap_ssid) - 1);
        s_ap_ssid[sizeof(s_ap_ssid) - 1] = '\0';
    }
    if (ap_pass && ap_pass[0] != '\0' && strcmp(ap_pass, "********") != 0) {
        prefs.putString("ap_pass", ap_pass);
        strncpy(s_ap_password, ap_pass, sizeof(s_ap_password) - 1);
        s_ap_password[sizeof(s_ap_password) - 1] = '\0';
    }
    if (ble_name && ble_name[0] != '\0') {
        prefs.putString("ble_name", ble_name);
        strncpy(s_ble_name, ble_name, sizeof(s_ble_name) - 1);
        s_ble_name[sizeof(s_ble_name) - 1] = '\0';
    }

    prefs.end();
    schedule_system_restart(1500);
}

void save_wifi_credentials(const char* sta_ssid, const char* sta_pass, const char* ap_ssid, const char* ap_pass) {
    save_all_device_config(sta_ssid, sta_pass, ap_ssid, ap_pass, NULL);
}

void switch_wifi_mode(const char* target_mode) {
    Preferences prefs;
    prefs.begin("lore_cfg", false);
    if (strcasecmp(target_mode, "AP") == 0) {
        prefs.putString("wifi_mode", "AP");
    } else if (strcasecmp(target_mode, "STA") == 0) {
        prefs.putString("wifi_mode", "STA");
    }
    prefs.end();
    schedule_system_restart(1500);
}
