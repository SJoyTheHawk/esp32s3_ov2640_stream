#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "camera_settings.h"

class CameraWebServer {
public:
    CameraWebServer(uint16_t port, CameraSettings* settings);
    ~CameraWebServer();

    void begin();
    void loop();

private:
    static const unsigned long COOKIE_TIMEOUT_MS = 1800000UL;

    AsyncWebServer server_;
    CameraSettings* settings_;
    String authToken_;
    unsigned long lastActivityMs_;

    String generateToken();
    bool isAuthenticated(AsyncWebServerRequest* request);
    void sendJson(AsyncWebServerRequest* request, int status, const char* message);
    void sendUnauthorized(AsyncWebServerRequest* request);
    void handleRoot(AsyncWebServerRequest* request);
    void handleLogin(AsyncWebServerRequest* request);
    void handleLogout(AsyncWebServerRequest* request);
    void handleChangePassword(AsyncWebServerRequest* request);
};

#endif // WEB_SERVER_H
