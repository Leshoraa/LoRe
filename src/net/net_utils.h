/**
 * @file net_utils.h
 * @brief Network parsing utilities, JSON string extraction, and notification classification.
 */

#ifndef LORE_NET_UTILS_H
#define LORE_NET_UTILS_H

#include "src/types/lore_types.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef ARDUINO
#include <WString.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool extract_json_field(const char* json, const char* key, char* out, size_t max_len);
void classify_notification_app(const char* title, const char* message, char* out_app, size_t max_len);
NavIconType parse_nav_icon_type(const char* icon_str);

#ifdef __cplusplus
}
#endif

#ifdef ARDUINO
inline bool extract_json_field(const String& json, const char* key, char* out, size_t max_len) {
    return extract_json_field(json.c_str(), key, out, max_len);
}
#endif

#endif /* LORE_NET_UTILS_H */
