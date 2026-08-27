#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "camera_settings.h"

class WiFiProvisioning {
public:
    explicit WiFiProvisioning(CameraSettings* settings);
    ~WiFiProvisioning();

    bool startAPMode();
    void stopAPMode();
    void handleDNS();
    bool isActive() const;
    bool timedOut() const;

private:
    static constexpr uint16_t DNS_PORT = 53;
    static constexpr unsigned long AP_TIMEOUT_MS = 10UL * 60UL * 1000UL;
    CameraSettings* settings_;
    DNSServer dnsServer_;
    AsyncWebServer server_;
    unsigned long startedAtMs_;
    bool active_;
    bool restartPending_;
    unsigned long restartAtMs_;
    AsyncWebServerRequest* configureRequest_;
    String pendingSSID_;
    String pendingWiFiPassword_;
    String pendingAdminUsername_;
    String pendingAdminPassword_;
    String pendingUserUsername_;
    String pendingUserPassword_;
    unsigned long connectionDeadlineMs_;
    bool connectionTestCompleted_;
    bool connectionTestSucceeded_;
    String connectionTestMessage_;

    void setupWebHandlers();
    String setupPage() const;
    String scanNetworks() const;
    void handleConfigure(AsyncWebServerRequest* request);
    void processConnection();
};

#endif
