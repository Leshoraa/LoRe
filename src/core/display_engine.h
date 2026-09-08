/**
 * @file display_engine.h
 * @brief LovyanGFX SSD1306 display driver and frame orchestrator for LoRe.
 */

#ifndef LORE_DISPLAY_ENGINE_H
#define LORE_DISPLAY_ENGINE_H

#include "src/config/lore_config.h"
#include "src/types/lore_types.h"
#include "src/core/gaze_engine.h"
#include "src/core/facial_renderer.h"
#include "src/core/ambient_screens.h"
#include <LovyanGFX.hpp>

/**
 * @class LGFX
 * @brief SSD1306 display driver subclass configured for 1.0 MHz Fast-Mode Plus I2C bus on ESP32-S3 SuperMini.
 */
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_SSD1306 _panel_instance;
    lgfx::Bus_I2C _bus_instance;

public:
    LGFX() {
        {
            auto cfg = _bus_instance.config();
            cfg.i2c_port = OLED_I2C_PORT;
            cfg.freq_write = OLED_I2C_FREQ_WRITE_HZ;
            cfg.freq_read = OLED_I2C_FREQ_READ_HZ;
            cfg.pin_scl = PIN_OLED_SCL;
            cfg.pin_sda = PIN_OLED_SDA;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.panel_width = OLED_PANEL_WIDTH_PX;
            cfg.panel_height = OLED_PANEL_HEIGHT_PX;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

extern LGFX lcd;
extern LGFX_Sprite canvas;

extern volatile bool g_auto_brightness_enabled;

int get_burn_shift_x(void);
int get_burn_shift_y(void);

void showBootStatus(const char* line1, const char* line2 = nullptr);

void triggerAmbientDisplay(AmbientScreenMode mode, uint32_t duration_ms = WEATHER_POPUP_DURATION_MS);
void triggerWeatherDisplay(uint32_t duration_ms = WEATHER_POPUP_DURATION_MS);
void triggerClockDisplay(uint32_t duration_ms = WEATHER_POPUP_DURATION_MS);
void triggerNotificationDisplay(const NotificationInfo& notif, uint32_t duration_ms = NOTIFICATION_POPUP_DURATION_MS);
void triggerNavigationDisplay(const NavigationInfo& nav, uint32_t duration_ms = 15000);
void dismissNavigationDisplay(void);

void setOledBrightnessLive(uint8_t brightness);
void setAutoBrightnessLive(bool enabled);

bool isOledPanelSleeping(void);
void wakeOledFromDeepSleep(uint32_t duration_ms = OLED_DEEP_SLEEP_WAKE_TIMEOUT_MS);

void oledTask(void* pvParameters);

#endif /* LORE_DISPLAY_ENGINE_H */
