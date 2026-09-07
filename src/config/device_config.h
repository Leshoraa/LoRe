/**
 * @file device_config.h
 * @brief Persistent device configuration, NVS storage, and system lifecycle management.
 */

#ifndef LORE_DEVICE_CONFIG_H
#define LORE_DEVICE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_device_config(void);

const char* get_wifi_sta_ssid(void);
const char* get_wifi_sta_pass(void);
const char* get_wifi_ap_ssid(void);
const char* get_wifi_ap_pass(void);
const char* get_ble_device_name(void);
const char* get_configured_wifi_mode(void);

void save_wifi_credentials(const char* sta_ssid, const char* sta_pass, const char* ap_ssid, const char* ap_pass);
void save_ble_config(const char* ble_name);
void save_all_device_config(const char* sta_ssid, const char* sta_pass, const char* ap_ssid, const char* ap_pass, const char* ble_name);
void switch_wifi_mode(const char* target_mode);

uint8_t get_saved_oled_brightness(void);
void save_oled_brightness(uint8_t brightness);
bool is_auto_brightness_enabled(void);
void save_auto_brightness_enabled(bool is_enabled);

const char* get_weather_city(void);
float get_weather_lat(void);
float get_weather_lon(void);
bool is_weather_enabled(void);
void save_weather_config(const char* city, float lat, float lon, bool is_enabled);

int32_t get_timezone_offset_sec(void);
void save_timezone_offset_sec(int32_t offset_sec);
void apply_timezone_config(void);

void schedule_system_restart(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* LORE_DEVICE_CONFIG_H */
