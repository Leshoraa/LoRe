/**
 * @file LoRe.ino
 * @brief Master firmware entrypoint and dual-core FreeRTOS task orchestrator for LoRe.
 * @details Target Platform: ESP32-S3 SuperMini
 *          Architecture: FreeRTOS Dual-Core Asynchronous Task Split
 *                        Core 0: Network, HTTP Server, BLE Companion, Ambient Services, DFS
 *                        Core 1: 60 FPS Biomechanical Gaze Kinematics & 1-Bit LGFX Sprite Engine
 */

#include "src/config/device_config.h"
#include "include/lore_config.h"
#include "include/lore_types.h"
#include "include/lore_kalman.h"
#include "include/lore_kinematics.h"
#include "include/lore_affective.h"
#include "include/lore_ai.h"
#include "include/lore_autonomic.h"
#include "include/lore_personality.h"
#include "src/core/gaze_engine.h"
#include "src/core/display_engine.h"
#include "src/net/wifi_manager.h"
#include "src/net/http_server.h"
#include "src/net/weather_client.h"
#include "src/net/notification_client.h"
#include "src/net/ble_manager.h"
#include <Arduino.h>

void setup() {
    /* Set CPU clock to active compute frequency (240 MHz) */
    setCpuFrequencyMhz(CPU_FREQ_ACTIVE_MHZ);

    /* Initialize device configuration and NVS parameters */
    init_device_config();

    /* Initialize OLED display to present boot progress */
    lcd.init();
    lcd.setRotation(2);
    lcd.setBrightness(OLED_DEFAULT_BRIGHTNESS);
    showBootStatus("LoRe Starting...");

    LORE_LOG_INF("MAIN", "LoRe Biomechanical Robotic Engine Starting");

    /* Initialize On-Device TinyML Micro-Brain and Homeostatic Drives */
    initBrainEngine();

    /* Initialize Autonomic Brainstem Dynamics (Matsuoka CPG & Homeostatic Soma) */
    initAutonomicEngine();

    /* Load persistent personality traits before any behavioral tick */
    initPersonalityEngine();

    /* Initialize Autonomous Gaze Engine (replaces camera pipeline) */
    initGazeEngine();

    /* Initialize background Bluetooth Low Energy (BLE) notification server early
     * to reserve contiguous internal SRAM before Wi-Fi fragments the heap */
    initBleNotificationServer();

    /* Initialize Wi-Fi subsystem and NVS configuration */
    initWiFiAndNetwork();

    /* Initialize background Open-Meteo weather client */
    initWeatherClient();

    /* Initialize background Ntfy.sh notification client */
    initNotificationClient();

    /* Start HTTP web dashboard (Port 80) */
    startWebServer();
    LORE_LOG_INF("MAIN", "HTTP services active on Port 80");

    /* Dispatch FreeRTOS Core 0 Gaze Coordination & Power Scaling Task (Priority 2) */
    xTaskCreatePinnedToCore(
        gazeTask,
        "Gaze_AI_Task",
        4096,
        NULL,
        2,
        NULL,
        0
    );

    /* Dispatch FreeRTOS Core 1 Display & Ocular Kinematics Task (Priority 1) */
    xTaskCreatePinnedToCore(
        oledTask,
        "OLED_Task",
        6144,
        NULL,
        1,
        NULL,
        1
    );

    LORE_LOG_INF("MAIN", "All FreeRTOS tasks dispatched successfully");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(5000));
}
