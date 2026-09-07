/**
 * @file ble_manager.h
 * @brief Bluetooth Low Energy (BLE) Nordic UART Service (NUS) GATT server for mobile notifications.
 */

#ifndef LORE_BLE_MANAGER_H
#define LORE_BLE_MANAGER_H

#include "src/config/lore_config.h"
#include "src/net/telemetry_service.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void initBleNotificationServer(void);
void sendBleTelemetryNow(void);
void setBleTelemetryStreaming(bool enable, uint32_t interval_ms);
bool isBleTelemetryStreaming(void);
bool isBleConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* LORE_BLE_MANAGER_H */
