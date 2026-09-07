/**
 * @file ambient_screens.h
 * @brief Clock, Weather, Phone Notification, and Navigation HUD ambient screen rendering.
 */

#ifndef LORE_AMBIENT_SCREENS_H
#define LORE_AMBIENT_SCREENS_H

#include "src/types/lore_types.h"
#include <LovyanGFX.hpp>

enum AmbientScreenMode {
    AMBIENT_NONE = 0,
    AMBIENT_CLOCK,
    AMBIENT_WEATHER,
    AMBIENT_NOTIFICATION,
    AMBIENT_NAVIGATION
};

void set_ambient_canvas(LGFX_Sprite* p_canvas);
void renderClockToCanvas(float animFrame, int offsetY = 0);
void drawClockScreen(float animFrame);
void renderWeatherToCanvas(const WeatherInfo& weather, float animFrame, int offsetY = 0);
void drawWeatherScreen(const WeatherInfo& weather, float animFrame);
void renderNotificationToCanvas(const NotificationInfo& notif, float animFrame, int offsetY = 0);
void drawNotificationScreen(const NotificationInfo& notif, float animFrame);
void renderNavigationToCanvas(const NavigationInfo& nav, float animFrame, int offsetY = 0);
void drawNavigationScreen(const NavigationInfo& nav, float animFrame);

void transitionToAmbient(AmbientScreenMode toMode, float durationMs = 160.0f);
void transitionFromAmbientToFace(AmbientScreenMode fromMode, Expression toExpr, float durationMs = 140.0f);

#endif /* LORE_AMBIENT_SCREENS_H */
