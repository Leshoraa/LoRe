/**
 * @file wifi_manager.h
 * @brief Wi-Fi STA connection, SoftAP fallback, mDNS, NetBIOS, and DNS captive portal.
 */

#ifndef LORE_WIFI_MANAGER_H
#define LORE_WIFI_MANAGER_H

#include "src/config/lore_config.h"
#include "src/config/device_config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool initWiFiAndNetwork(void);
bool isWiFiAPMode(void);
bool isWiFiConnected(void);

#ifdef __cplusplus
}
#endif

/* Backward-compatible inline wrappers delegating to device_config */
inline const char* getWiFiStaSSID(void) { return get_wifi_sta_ssid(); }
inline const char* getWiFiStaPass(void) { return get_wifi_sta_pass(); }
inline const char* getWiFiApSSID(void) { return get_wifi_ap_ssid(); }
inline const char* getWiFiApPass(void) { return get_wifi_ap_pass(); }
inline const char* getBleDeviceName(void) { return get_ble_device_name(); }
inline void saveWiFiCredentials(const char* sta_s, const char* sta_p, const char* ap_s, const char* ap_p) {
    save_wifi_credentials(sta_s, sta_p, ap_s, ap_p);
}
inline void saveBleConfig(const char* name) { save_ble_config(name); }
inline void saveAllDeviceConfig(const char* sta_s, const char* sta_p, const char* ap_s, const char* ap_p, const char* ble_name) {
    save_all_device_config(sta_s, sta_p, ap_s, ap_p, ble_name);
}
inline void switchWiFiMode(const char* target_mode) { switch_wifi_mode(target_mode); }
inline void scheduleSystemRestart(uint32_t delay_ms) { schedule_system_restart(delay_ms); }

inline uint8_t getSavedOledBrightness(void) { return get_saved_oled_brightness(); }
inline void saveOledBrightness(uint8_t brightness) { save_oled_brightness(brightness); }
inline bool isAutoBrightnessEnabled(void) { return is_auto_brightness_enabled(); }
inline void saveAutoBrightnessEnabled(bool enabled) { save_auto_brightness_enabled(enabled); }
inline const char* getWeatherCity(void) { return get_weather_city(); }
inline float getWeatherLat(void) { return get_weather_lat(); }
inline float getWeatherLon(void) { return get_weather_lon(); }
inline bool isWeatherEnabled(void) { return is_weather_enabled(); }
inline void saveWeatherConfig(const char* city, float lat, float lon, bool enabled) {
    save_weather_config(city, lat, lon, enabled);
}
inline int32_t getTimezoneOffsetSec(void) { return get_timezone_offset_sec(); }
inline void saveTimezoneOffsetSec(int32_t offset_sec) { save_timezone_offset_sec(offset_sec); }
inline void applyTimezoneConfig(void) { apply_timezone_config(); }

#endif /* LORE_WIFI_MANAGER_H */
