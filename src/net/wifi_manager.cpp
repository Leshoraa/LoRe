/**
 * @file wifi_manager.cpp
 * @brief Wi-Fi STA connection, SoftAP fallback, mDNS, NetBIOS, and DNS captive portal implementation.
 */

#include "src/net/wifi_manager.h"
#include "src/config/device_config.h"
#include "src/config/lore_config.h"
#include "src/core/display_engine.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetBIOS.h>
#include <DNSServer.h>
#include <Arduino.h>

static bool s_is_ap_mode = false;
static DNSServer s_dnsServer;

bool isWiFiAPMode(void) {
    return s_is_ap_mode;
}

bool isWiFiConnected(void) {
    return (WiFi.status() == WL_CONNECTED);
}

bool initWiFiAndNetwork(void) {
    init_device_config();

    const char* sta_ssid = get_wifi_sta_ssid();
    const char* sta_pass = get_wifi_sta_pass();
    const char* ap_ssid = get_wifi_ap_ssid();
    const char* ap_pass = get_wifi_ap_pass();
    const char* wifi_mode = get_configured_wifi_mode();

    bool force_ap = (strcmp(wifi_mode, "AP") == 0);
    bool connected = false;

    if (!force_ap && strlen(sta_ssid) > 0) {
        WiFi.mode(WIFI_STA);
        delay(100);
        WiFi.disconnect(true);
        delay(150);
        WiFi.setSleep(WIFI_PS_NONE);
        WiFi.setAutoReconnect(true);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        char wifi_msg[40];
        snprintf(wifi_msg, sizeof(wifi_msg), "WiFi: %s", sta_ssid);
        showBootStatus(wifi_msg, "Connecting...");

        if (strlen(sta_pass) > 0) {
            WiFi.begin(sta_ssid, sta_pass);
        } else {
            WiFi.begin(sta_ssid);
        }

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 25) {
            delay(300);
            if (attempts % 4 == 0) {
                char dots[16] = "Connecting";
                int d = (attempts / 4) % 3;
                for (int i = 0; i <= d; i++) strcat(dots, ".");
                showBootStatus(wifi_msg, dots);
            }
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
        }
    }

    if (connected) {
        s_is_ap_mode = false;
        apply_timezone_config();

        if (MDNS.begin("lore")) {
            MDNS.setInstanceName("LoRe Robot");
            MDNS.addService("http", "tcp", HTTP_PORT_WEB_CONTROL);
        }

        NBNS.begin("lore");

        char sta_ip_buf[40];
        snprintf(sta_ip_buf, sizeof(sta_ip_buf), "IP: %s", WiFi.localIP().toString().c_str());
        showBootStatus("WiFi Connected!", sta_ip_buf);
        delay(1500);
        return true;
    } else {
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_AP);
        WiFi.setSleep(WIFI_PS_NONE);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        IPAddress apIP(192, 168, 18, 16);
        IPAddress apGateway(192, 168, 18, 16);
        IPAddress apSubnet(255, 255, 255, 0);
        WiFi.softAPConfig(apIP, apGateway, apSubnet);

        if (strlen(ap_pass) > 0 && strlen(ap_pass) < 8) {
            WiFi.softAP(ap_ssid, NULL);
        } else if (strlen(ap_pass) == 0) {
            WiFi.softAP(ap_ssid, NULL);
        } else {
            WiFi.softAP(ap_ssid, ap_pass);
        }

        if (MDNS.begin("lore")) {
            MDNS.setInstanceName("LoRe Robot AP");
            MDNS.addService("http", "tcp", HTTP_PORT_WEB_CONTROL);
        }

        NBNS.begin("lore");

        s_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        s_dnsServer.start(53, "*", WiFi.softAPIP());
        s_is_ap_mode = true;

        xTaskCreatePinnedToCore([](void* arg) {
            (void)arg;
            while (true) {
                if (s_is_ap_mode) {
                    s_dnsServer.processNextRequest();
                    vTaskDelay(pdMS_TO_TICKS(5));
                } else {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }, "AP_DNS_Task", 2048, NULL, 1, NULL, 0);

        char ap_ip_buf[40];
        if (!force_ap && strlen(sta_ssid) > 0) {
            showBootStatus("STA Fail -> AP Mode", WiFi.softAPIP().toString().c_str());
        } else {
            snprintf(ap_ip_buf, sizeof(ap_ip_buf), "AP IP: %s", WiFi.softAPIP().toString().c_str());
            showBootStatus("AP Mode Active", ap_ip_buf);
        }
        delay(1500);
        return false;
    }
}
