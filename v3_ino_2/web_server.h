#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <functional>
#include "camera_settings.h"
#include "weight_sensor.h"
#include "ws2812b_controller.h"

class CameraWebServer {
public:
    enum class AuthLevel { NONE, USER, ADMIN };
    CameraWebServer(uint16_t port, CameraSettings* settings);
    ~CameraWebServer();

    void begin();
    void loop();
    void setFirmwareVersion(const char* version);
    void setReconnectCallback(std::function<void()> callback);
    void setFrameCaptureCallback(std::function<size_t(uint8_t*, size_t, WeightReading&)> callback);
    void setWeightSensor(WeightSensor* sensor);
    void setScaleEnabledCallback(std::function<void(bool)> callback);
    void setFrameRateCallback(std::function<uint8_t()> callback);
    void setCameraConfigCallback(std::function<bool(uint8_t, uint8_t, int8_t, int8_t, int8_t, bool, bool)> callback);
    void setLedController(Ws2812bController* controller);
    void setIndicatorController(Ws2812bController* controller);

private:
    static const unsigned long COOKIE_TIMEOUT_MS = 1800000UL;

    AsyncWebServer server_;
    CameraSettings* settings_;
    const char* firmwareVersion_ = "unknown";
    String adminAuthToken_;
    String userAuthToken_;
    unsigned long adminLastActivityMs_;
    unsigned long userLastActivityMs_;
    unsigned long reconnectAtMs_;
    std::function<void()> reconnectCallback_;
    std::function<size_t(uint8_t*, size_t, WeightReading&)> frameCaptureCallback_;
    WeightSensor* weightSensor_ = nullptr;
    std::function<void(bool)> scaleEnabledCallback_;
    std::function<uint8_t()> frameRateCallback_;
    std::function<bool(uint8_t, uint8_t, int8_t, int8_t, int8_t, bool, bool)> cameraConfigCallback_;
    Ws2812bController* ledController_ = nullptr;
    Ws2812bController* indicatorController_ = nullptr;
    SemaphoreHandle_t ledApiMutex_ = nullptr;

    String generateToken();
    AuthLevel getAuthLevel(AsyncWebServerRequest* request);
    bool isAuthenticated(AsyncWebServerRequest* request);
    bool isAdminAuthenticated(AsyncWebServerRequest* request);
    void sendJson(AsyncWebServerRequest* request, int status, const char* message);
    void sendUnauthorized(AsyncWebServerRequest* request);
    void handleRoot(AsyncWebServerRequest* request);
    void handleLogin(AsyncWebServerRequest* request);
    void handleLogout(AsyncWebServerRequest* request);
    void handleChangePassword(AsyncWebServerRequest* request);
    void handleGetSettings(AsyncWebServerRequest* request);
    void handlePostSettings(AsyncWebServerRequest* request);
    void handleScaleEnabled(AsyncWebServerRequest* request);
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleCameraConfig(AsyncWebServerRequest* request);
    void handleStream(AsyncWebServerRequest* request);
    void handleCapture(AsyncWebServerRequest* request);
    void handleGetWeight(AsyncWebServerRequest* request);
    void handleTareWeight(AsyncWebServerRequest* request);
    void handleCalibrateWeight(AsyncWebServerRequest* request);
    void appendLedState(JsonObject object);
    void handleGetLed(AsyncWebServerRequest* request);
    void handleLedControl(AsyncWebServerRequest* request);
    void handleLedHardware(AsyncWebServerRequest* request);
    void indicateSuccess();
    void indicateUnmatched();
    bool parseIPAddress(const String& value, byte destination[4]);
};

#endif // WEB_SERVER_H
