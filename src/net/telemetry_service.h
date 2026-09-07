/**
 * @file telemetry_service.h
 * @brief Telemetry snapshot serialization service for BLE and HTTP Web APIs.
 */

#ifndef LORE_TELEMETRY_SERVICE_H
#define LORE_TELEMETRY_SERVICE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void formatTelemetryJson(char* json, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* LORE_TELEMETRY_SERVICE_H */
