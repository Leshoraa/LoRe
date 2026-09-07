/**
 * @file env.example.h
 * @brief Template for environment-specific credentials and network configuration for LoRe.
 *
 * Copy this file to env.h and configure your local Wi-Fi credentials:
 *   cp src/config/env.example.h src/config/env.h
 */

#ifndef LORE_ENV_H
#define LORE_ENV_H

/* Default Wi-Fi Station (STA) Credentials */
#define WIFI_STA_DEFAULT_SSID     "YOUR_WIFI_SSID"
#define WIFI_STA_DEFAULT_PASS     "YOUR_WIFI_PASSWORD"

/* Fallback Access Point (AP) Configuration */
#define WIFI_FALLBACK_AP_SSID     "LoRe-AP"
#define WIFI_FALLBACK_AP_PASS     "12345678"

/* Bluetooth Low Energy (BLE) Configuration */
#define BLE_DEVICE_NAME_DEFAULT   "LoRe-SuperMini"

/* Push Notifications & Cloud Topic Configuration */
#define NTFY_DEFAULT_TOPIC        "lore_notif_default"

#endif /* LORE_ENV_H */
