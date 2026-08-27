#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <functional>
#include "camera_settings.h"

class CameraWebServer {
public:
    CameraWebServer(uint16_t port, CameraSettings* settings);
    ~CameraWebServer();

    void begin();
    void loop();
    void setReconnectCallback(std::function<void()> callback);
    void setFrameCaptureCallback(std::function<size_t(uint8_t*, size_t)> callback);
    void setFrameRateCallback(std::function<uint8_t()> callback);
    void setCameraConfigCallback(std::function<bool(uint8_t, uint8_t, int8_t, int8_t, int8_t, bool, bool)> callback);

private:
    static const unsigned long COOKIE_TIMEOUT_MS = 1800000UL;

    AsyncWebServer server_;
    CameraSettings* settings_;
    String authToken_;
    unsigned long lastActivityMs_;
    unsigned long reconnectAtMs_;
    std::function<void()> reconnectCallback_;
    std::function<size_t(uint8_t*, size_t)> frameCaptureCallback_;
    std::function<uint8_t()> frameRateCallback_;
    std::function<bool(uint8_t, uint8_t, int8_t, int8_t, int8_t, bool, bool)> cameraConfigCallback_;

    String generateToken();
    bool isAuthenticated(AsyncWebServerRequest* request);
    void sendJson(AsyncWebServerRequest* request, int status, const char* message);
    void sendUnauthorized(AsyncWebServerRequest* request);
    void handleRoot(AsyncWebServerRequest* request);
    void handleLogin(AsyncWebServerRequest* request);
    void handleLogout(AsyncWebServerRequest* request);
    void handleChangePassword(AsyncWebServerRequest* request);
    void handleGetSettings(AsyncWebServerRequest* request);
    void handlePostSettings(AsyncWebServerRequest* request);
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleCameraConfig(AsyncWebServerRequest* request);
    void handleStream(AsyncWebServerRequest* request);
    void handleCapture(AsyncWebServerRequest* request);
    bool parseIPAddress(const String& value, byte destination[4]);
};

#endif // WEB_SERVER_H
