#include "web_server.h"

#include <ArduinoJson.h>
#include "html_pages.h"

CameraWebServer::CameraWebServer(uint16_t port, CameraSettings* settings)
    : server_(port), settings_(settings), lastActivityMs_(0) {
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
