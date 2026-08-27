#include "web_server.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstdlib>
#include "html_pages.h"

CameraWebServer::CameraWebServer(uint16_t port, CameraSettings* settings)
    : server_(port), settings_(settings), lastActivityMs_(0), reconnectAtMs_(0) {
    randomSeed(static_cast<unsigned long>(micros()));
}

CameraWebServer::~CameraWebServer() {
    server_.end();
}

void CameraWebServer::begin() {
    server_.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    server_.on("/api/login", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleLogin(request);
    });
    server_.on("/api/logout", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleLogout(request);
    });
    server_.on("/api/change-password", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleChangePassword(request);
    });
    server_.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetSettings(request);
    });
    server_.on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handlePostSettings(request);
    });
    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    server_.on("/api/camera/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleCameraConfig(request);
    });
    server_.on("/stream", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleStream(request);
    });
    server_.on("/capture", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCapture(request);
    });
    server_.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "404 Not Found");
    });
    server_.begin();
    Serial.println("[WEB] Server started on port 80");
}

void CameraWebServer::loop() {
    if (authToken_.length() > 0 &&
        static_cast<unsigned long>(millis() - lastActivityMs_) >= COOKIE_TIMEOUT_MS) {
        authToken_ = "";
        lastActivityMs_ = 0;
        Serial.println("[WEB] Session expired");
    }

    if (reconnectAtMs_ != 0 &&
        static_cast<long>(millis() - reconnectAtMs_) >= 0) {
        reconnectAtMs_ = 0;
        if (reconnectCallback_) {
            Serial.println("[WEB] Applying updated network settings");
            reconnectCallback_();
        }
    }
}

void CameraWebServer::setReconnectCallback(std::function<void()> callback) {
    reconnectCallback_ = callback;
}

void CameraWebServer::setFrameCaptureCallback(std::function<size_t(uint8_t*, size_t)> callback) {
    frameCaptureCallback_ = callback;
}

void CameraWebServer::setFrameRateCallback(std::function<uint8_t()> callback) {
    frameRateCallback_ = callback;
}

void CameraWebServer::setCameraConfigCallback(std::function<bool(uint8_t, uint8_t, int8_t, int8_t, int8_t, bool, bool)> callback) {
    cameraConfigCallback_ = callback;
}

String CameraWebServer::generateToken() {
    static const char alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    String token;
    token.reserve(32);
    for (uint8_t i = 0; i < 32; ++i) {
        token += alphabet[random(0, sizeof(alphabet) - 1)];
    }
    return token;
}

bool CameraWebServer::isAuthenticated(AsyncWebServerRequest* request) {
    if (authToken_.length() == 0 || !request->hasHeader("Cookie")) {
        return false;
    }

    const String cookie = request->header("Cookie");
    const int start = cookie.indexOf("auth_token=");
    if (start < 0) return false;

    const int valueStart = start + 11;
    int valueEnd = cookie.indexOf(';', valueStart);
    if (valueEnd < 0) valueEnd = cookie.length();
    const String token = cookie.substring(valueStart, valueEnd);
    if (token != authToken_ || static_cast<unsigned long>(millis() - lastActivityMs_) >= COOKIE_TIMEOUT_MS) {
        return false;
    }

    lastActivityMs_ = millis();
    return true;
}

void CameraWebServer::sendJson(AsyncWebServerRequest* request, int status, const char* message) {
    StaticJsonDocument<192> document;
    document["status"] = status >= 200 && status < 300 ? "success" : "error";
    document["message"] = message;
    String body;
    serializeJson(document, body);
    request->send(status, "application/json", body);
}

void CameraWebServer::sendUnauthorized(AsyncWebServerRequest* request) {
    sendJson(request, 401, "Unauthorized");
}

void CameraWebServer::handleRoot(AsyncWebServerRequest* request) {
    request->send(200, "text/html", isAuthenticated(request) ? CameraHtml::MAIN : CameraHtml::LOGIN);
}

void CameraWebServer::handleLogin(AsyncWebServerRequest* request) {
    if (!settings_ || !request->hasArg("username") || !request->hasArg("password")) {
        sendJson(request, 400, "Missing username or password");
        return;
    }

    const String username = request->arg("username");
    const String password = request->arg("password");
    if (username != settings_->username || password != settings_->password) {
        sendJson(request, 401, "Invalid username or password");
        return;
    }

    authToken_ = generateToken();
    lastActivityMs_ = millis();
    StaticJsonDocument<256> document;
    document["status"] = "success";
    document["message"] = "Login successful";
    document["expires_in"] = COOKIE_TIMEOUT_MS / 1000UL;
    String body;
    serializeJson(document, body);

    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", body);
    response->addHeader("Set-Cookie", "auth_token=" + authToken_ +
                        "; Max-Age=" + String(COOKIE_TIMEOUT_MS / 1000UL) +
                        "; Path=/; HttpOnly; SameSite=Strict");
    request->send(response);
    Serial.printf("[WEB] User '%s' logged in\n", username.c_str());
}

void CameraWebServer::handleLogout(AsyncWebServerRequest* request) {
    authToken_ = "";
    lastActivityMs_ = 0;
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json",
                                                               "{\"status\":\"success\",\"message\":\"Logged out\"}");
    response->addHeader("Set-Cookie", "auth_token=; Max-Age=0; Path=/; HttpOnly; SameSite=Strict");
    request->send(response);
    Serial.println("[WEB] User logged out");
}

void CameraWebServer::handleChangePassword(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        sendUnauthorized(request);
        return;
    }
    if (!request->hasArg("current_password") || !request->hasArg("new_password")) {
        sendJson(request, 400, "Missing current or new password");
        return;
    }

    const String current = request->arg("current_password");
    const String next = request->arg("new_password");
    if (current != String(settings_->password)) {
        sendJson(request, 401, "Current password is incorrect");
        return;
    }
    if (next.length() < 4 || next.length() > 31) {
        sendJson(request, 400, "Password must be 4-31 characters");
        return;
    }
    if (!settings_->writePassword(next.c_str(), next.length())) {
        sendJson(request, 500, "Failed to save password");
        return;
    }
    sendJson(request, 200, "Password changed successfully");
    Serial.println("[WEB] Password changed");
}

bool CameraWebServer::parseIPAddress(const String& value, byte destination[4]) {
    IPAddress address;
    if (!destination || !address.fromString(value)) return false;
    for (uint8_t i = 0; i < 4; ++i) destination[i] = address[i];
    return true;
}

void CameraWebServer::handleGetSettings(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        sendUnauthorized(request);
        return;
    }

    StaticJsonDocument<512> document;
    document["wifi_ssid"] = settings_->wifiSSID;
    document["wifi_password_set"] = settings_->wifiPassword[0] != '\0';
    document["use_dhcp"] = settings_->useDHCP;
    document["static_ip"] = IPAddress(settings_->staticIP).toString();
    document["gateway"] = IPAddress(settings_->gateway).toString();
    document["subnet"] = IPAddress(settings_->subnet).toString();
    document["device_name"] = settings_->deviceName;
    document["camera_resolution"] = settings_->cameraResolution;
    document["camera_quality"] = settings_->cameraQuality;
    document["frame_rate"] = settings_->frameRate;
    document["brightness"] = settings_->brightness;
    document["contrast"] = settings_->contrast;
    document["saturation"] = settings_->saturation;
    document["vertical_flip"] = settings_->verticalFlip;
    document["horizontal_mirror"] = settings_->horizontalMirror;

    String body;
    serializeJson(document, body);
    request->send(200, "application/json", body);
}

void CameraWebServer::handlePostSettings(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        sendUnauthorized(request);
        return;
    }
    if (!request->hasArg("wifi_ssid") || !request->hasArg("use_dhcp")) {
        sendJson(request, 400, "Missing WiFi SSID or DHCP setting");
        return;
    }

    const String ssid = request->arg("wifi_ssid");
    const String password = request->hasArg("wifi_password")
                                ? request->arg("wifi_password")
                                : String();
    const bool clearPassword = request->hasArg("clear_wifi_password") &&
                               request->arg("clear_wifi_password") == "true";
    const String dhcpValue = request->arg("use_dhcp");
    if (ssid.length() == 0 || ssid.length() > 31) {
        sendJson(request, 400, "WiFi SSID must be 1-31 characters");
        return;
    }
    if (password.length() > 31) {
        sendJson(request, 400, "WiFi password must be at most 31 characters");
        return;
    }
    if (dhcpValue != "true" && dhcpValue != "false") {
        sendJson(request, 400, "Invalid DHCP setting");
        return;
    }

    const bool useDHCP = dhcpValue == "true";
    byte staticIP[4];
    byte gateway[4];
    byte subnet[4];
    memcpy(staticIP, settings_->staticIP, sizeof(staticIP));
    memcpy(gateway, settings_->gateway, sizeof(gateway));
    memcpy(subnet, settings_->subnet, sizeof(subnet));
    const bool hasStaticFields = request->hasArg("static_ip") &&
                                 request->hasArg("gateway") &&
                                 request->hasArg("subnet");
    if ((!useDHCP && !hasStaticFields) ||
        (hasStaticFields &&
         (!parseIPAddress(request->arg("static_ip"), staticIP) ||
          !parseIPAddress(request->arg("gateway"), gateway) ||
          !parseIPAddress(request->arg("subnet"), subnet)))) {
        sendJson(request, 400, "Static IP, gateway, and subnet must be valid IPv4 addresses");
        return;
    }

    const String effectivePassword = clearPassword
                                         ? String()
                                         : (password.length() > 0
                                                ? password
                                                : String(settings_->wifiPassword));
    const String oldSSID(settings_->wifiSSID);
    const String oldPassword(settings_->wifiPassword);
    if (!settings_->writeWiFiSettings(ssid.c_str(), ssid.length(),
                                      effectivePassword.c_str(), effectivePassword.length())) {
        sendJson(request, 500, "Failed to save WiFi settings");
        return;
    }
    if (!settings_->writeNetworkSettings(useDHCP, staticIP, gateway, subnet)) {
        settings_->writeWiFiSettings(oldSSID.c_str(), oldSSID.length(),
                                     oldPassword.c_str(), oldPassword.length());
        sendJson(request, 500, "Failed to save network settings");
        return;
    }

    StaticJsonDocument<256> document;
    document["status"] = "success";
    document["message"] = "Settings saved; reconnecting WiFi";
    document["reconnecting"] = reconnectCallback_ != nullptr;
    document["expected_ip"] = useDHCP ? "DHCP assigned" : IPAddress(staticIP).toString();
    String body;
    serializeJson(document, body);
    request->send(200, "application/json", body);

    if (reconnectCallback_) reconnectAtMs_ = millis() + 1500UL;
    Serial.printf("[WEB] Network settings saved (SSID: %s, DHCP: %s)\n",
                  ssid.c_str(), useDHCP ? "yes" : "no");
}

void CameraWebServer::handleGetStatus(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        sendUnauthorized(request);
        return;
    }

    const bool connected = WiFi.status() == WL_CONNECTED;
    StaticJsonDocument<384> document;
    document["uptime_seconds"] = millis() / 1000UL;
    document["wifi_connected"] = connected;
    document["ip_address"] = connected ? WiFi.localIP().toString() : "";
    document["wifi_ssid"] = connected ? WiFi.SSID() : settings_->wifiSSID;
    document["rssi"] = connected ? WiFi.RSSI() : 0;
    document["device_name"] = settings_->deviceName;
    String body;
    serializeJson(document, body);
    request->send(200, "application/json", body);
}

void CameraWebServer::handleCameraConfig(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        sendUnauthorized(request);
        return;
    }
    if (!cameraConfigCallback_ || !request->hasArg("resolution") ||
        !request->hasArg("quality") || !request->hasArg("frame_rate") ||
        !request->hasArg("brightness") || !request->hasArg("contrast") ||
        !request->hasArg("saturation") || !request->hasArg("vertical_flip") ||
        !request->hasArg("horizontal_mirror")) {
        sendJson(request, 400, "Missing camera settings");
        return;
    }

    const int resolution = request->arg("resolution").toInt();
    const int quality = request->arg("quality").toInt();
    const int frameRate = request->arg("frame_rate").toInt();
    const int brightness = request->arg("brightness").toInt();
    const int contrast = request->arg("contrast").toInt();
    const int saturation = request->arg("saturation").toInt();
    const String flip = request->arg("vertical_flip");
    const String mirror = request->arg("horizontal_mirror");
    if (resolution < 8 || resolution > 12 || quality < 10 || quality > 63 ||
        (frameRate != 5 && frameRate != 10 && frameRate != 15 && frameRate != 20) ||
        brightness < -2 || brightness > 2 || contrast < -2 || contrast > 2 ||
        saturation < -2 || saturation > 2 ||
        (flip != "true" && flip != "false") || (mirror != "true" && mirror != "false")) {
        sendJson(request, 400, "Invalid camera setting range");
        return;
    }

    if (!cameraConfigCallback_(static_cast<uint8_t>(resolution), static_cast<uint8_t>(quality),
                               static_cast<int8_t>(brightness), static_cast<int8_t>(contrast),
                               static_cast<int8_t>(saturation), flip == "true", mirror == "true")) {
        sendJson(request, 500, "Failed to apply camera settings");
        return;
    }
    if (!settings_->writeCameraResolution(static_cast<uint8_t>(resolution)) ||
        !settings_->writeCameraQuality(static_cast<uint8_t>(quality)) ||
        !settings_->writeFrameRate(static_cast<uint8_t>(frameRate)) ||
        !settings_->writeBrightness(static_cast<int8_t>(brightness)) ||
        !settings_->writeContrast(static_cast<int8_t>(contrast)) ||
        !settings_->writeSaturation(static_cast<int8_t>(saturation)) ||
        !settings_->writeVerticalFlip(flip == "true") ||
        !settings_->writeHorizontalMirror(mirror == "true")) {
        sendJson(request, 500, "Camera applied but persistence failed");
        return;
    }
    sendJson(request, 200, "Camera settings applied");
}

void CameraWebServer::handleCapture(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        sendUnauthorized(request);
        return;
    }
    if (!frameCaptureCallback_) {
        sendJson(request, 503, "Camera capture is unavailable");
        return;
    }

    // UXGA JPEGs can be larger than the default async response buffer. Copy
    // into PSRAM/heap memory so the camera frame can be returned immediately.
    const size_t capacity = 512 * 1024;
    uint8_t* frame = static_cast<uint8_t*>(malloc(capacity));
    if (!frame) {
        sendJson(request, 503, "Insufficient memory for capture");
        return;
    }
    const size_t length = frameCaptureCallback_(frame, capacity);
    if (length == 0 || length > capacity) {
        free(frame);
        sendJson(request, 503, "Camera capture failed");
        return;
    }
    AsyncWebServerResponse* response = request->beginResponse_P(200, "image/jpeg", frame, length);
    response->addHeader("Cache-Control", "no-store");
    response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
    response->addHeader("Connection", "close");
    request->onDisconnect([frame]() { free(frame); });
    request->send(response);
}

void CameraWebServer::handleStream(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        sendUnauthorized(request);
        return;
    }
    if (!frameCaptureCallback_) {
        sendJson(request, 503, "Camera capture is unavailable");
        return;
    }

    struct StreamState {
        uint8_t* frame = nullptr;
        size_t frameLength = 0;
        size_t frameOffset = 0;
        String prefix;
        size_t prefixOffset = 0;
        const char* suffix = "\r\n";
        size_t suffixOffset = 2;
        unsigned long nextFrameAt = 0;
    };
    StreamState* state = new StreamState();
    const std::function<size_t(uint8_t*, size_t)> capture = frameCaptureCallback_;
    const std::function<uint8_t()> fps = frameRateCallback_;

    AsyncWebServerResponse* response = request->beginChunkedResponse(
        "multipart/x-mixed-replace; boundary=frame",
        [state, capture, fps](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
            if (index == 0 && state->nextFrameAt == 0) state->nextFrameAt = millis();
            if (state->prefixOffset == state->prefix.length() &&
                state->frameOffset == state->frameLength &&
                state->suffixOffset == strlen(state->suffix)) {
                const uint8_t requestedFps = fps ? fps() : 10;
                const unsigned long interval = 1000UL / (requestedFps == 0 ? 10 : requestedFps);
                if (static_cast<long>(millis() - state->nextFrameAt) < 0)
                    delay(static_cast<unsigned long>(state->nextFrameAt - millis()));
                free(state->frame);
                state->frame = static_cast<uint8_t*>(malloc(512 * 1024));
                if (!state->frame) return 0;
                state->frameLength = capture(state->frame, 512 * 1024);
                if (state->frameLength == 0 || state->frameLength > 512 * 1024) return 0;
                state->frameOffset = 0;
                state->prefix = String("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: ") +
                                 String(state->frameLength) + "\r\n\r\n";
                state->prefixOffset = 0;
                state->suffixOffset = 0;
                state->nextFrameAt = millis() + interval;
            }

            size_t written = 0;
            while (written < maxLen && state->prefixOffset < state->prefix.length())
                buffer[written++] = state->prefix[state->prefixOffset++];
            while (written < maxLen && state->frameOffset < state->frameLength)
                buffer[written++] = state->frame[state->frameOffset++];
            while (written < maxLen && state->suffixOffset < strlen(state->suffix))
                buffer[written++] = state->suffix[state->suffixOffset++];
            return written;
        });
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Connection", "keep-alive");
    request->onDisconnect([state]() {
        free(state->frame);
        delete state;
    });
    request->send(response);
}
