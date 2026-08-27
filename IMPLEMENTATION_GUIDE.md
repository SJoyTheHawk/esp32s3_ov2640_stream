# ESP32-S3 Camera Implementation Guide

**For AI Assistant:** This guide provides detailed code examples and step-by-step instructions to implement the ESP32-S3 camera system based on the Gate_V1.0 architecture pattern.

---

## 📋 Prerequisites

**Read these files first to understand the context:**
1. `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/IMPLEMENTATION_PLAN.md` - Overall plan
2. `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/v3_ino_2/v3_ino_2.ino` - Current implementation
3. `/Users/szemy/Workspace/Gate/Gate_V1.0/Gate_V1.0.ino` - Reference architecture
4. `/Users/szemy/Workspace/Gate/Gate_V1.0/setting.h` - Reference settings class
5. `/Users/szemy/Workspace/Gate/Gate_V1.0/api_server.h` - Reference web server

**Key Differences from Gate:**
- Replace EEPROM with Preferences (NVS)
- Remove Ethernet code (WiFi only for now)
- Add camera-specific settings and controls
- Add MJPEG streaming endpoint
- Adapt HTML UI for camera preview and settings

---

## Phase 1: Storage Migration (EEPROM → NVS)

### Step 1.1: Create `camera_settings.h`

**Location:** `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/v3_ino_2/camera_settings.h`

```cpp
#ifndef __CAMERA_SETTINGS_H__
#define __CAMERA_SETTINGS_H__

#include <Arduino.h>
#include <Preferences.h>

class CameraSettings {
public:
    struct DefaultValues {
        // Authentication
        static constexpr char USERNAME[32] = "admin";
        static constexpr char PASSWORD[32] = "admin";
        
        // Network settings
        static constexpr bool USE_DHCP = true;
        static constexpr byte STATIC_IP[4] = {192, 168, 2, 100};
        static constexpr byte GATEWAY[4] = {192, 168, 2, 1};
        static constexpr byte SUBNET[4] = {255, 255, 255, 0};
        
        // WiFi settings
        static constexpr char WIFI_SSID[32] = "XIMS2";
        static constexpr char WIFI_PASSWORD[32] = "Ns203Ns203.";
        
        // Camera settings
        static constexpr uint8_t CAMERA_RESOLUTION = 10; // FRAMESIZE_SVGA
        static constexpr uint8_t CAMERA_QUALITY = 12;
        static constexpr uint8_t FRAME_RATE = 20;
        static constexpr int8_t BRIGHTNESS = 0;
        static constexpr int8_t CONTRAST = 0;
        static constexpr int8_t SATURATION = 0;
        static constexpr bool VERTICAL_FLIP = false;
        static constexpr bool HORIZONTAL_MIRROR = false;
        
        // Python server settings (optional)
        static constexpr bool PYTHON_SERVER_ENABLED = false;
        static constexpr char PYTHON_SERVER_IP[32] = "192.168.2.51";
        static constexpr uint16_t PYTHON_SERVER_PORT = 8000;
        
        // System
        static constexpr char DEVICE_NAME[32] = "ESP32-Camera";
        static constexpr char MDNS_HOSTNAME[32] = "camera";
    };
    
    // Authentication
    char username[32];
    char password[32];
    
    // Network settings
    bool useDHCP;
    byte staticIP[4];
    byte gateway[4];
    byte subnet[4];
    
    // WiFi settings
    char wifiSSID[32];
    char wifiPassword[32];
    
    // Camera settings
    uint8_t cameraResolution;
    uint8_t cameraQuality;
    uint8_t frameRate;
    int8_t brightness;
    int8_t contrast;
    int8_t saturation;
    bool verticalFlip;
    bool horizontalMirror;
    
    // Python server settings
    bool pythonServerEnabled;
    char pythonServerIP[32];
    uint16_t pythonServerPort;
    
    // System
    char deviceName[32];
    char mdnsHostname[32];
    
    CameraSettings();
    ~CameraSettings();
    
    // Initialization
    bool isNVSInitialized();
    bool initializeNVS();
    bool resetToDefault();
    
    // Read all settings from NVS
    void readFromNVS();
    
    // Write individual settings
    bool writeUsername(const char* user, size_t length);
    bool writePassword(const char* pass, size_t length);
    
    bool writeNetworkSettings(bool dhcp, byte ip[4], byte gw[4], byte sn[4]);
    bool writeDHCPSetting(bool dhcp);
    bool writeStaticIP(byte ip[4]);
    bool writeGateway(byte gw[4]);
    bool writeSubnet(byte sn[4]);
    
    bool writeWiFiSettings(const char* ssid, size_t ssidLen, const char* pass, size_t passLen);
    bool writeSSID(const char* ssid, size_t length);
    bool writeWiFiPassword(const char* pass, size_t length);
    
    bool writeCameraResolution(uint8_t resolution);
    bool writeCameraQuality(uint8_t quality);
    bool writeFrameRate(uint8_t fps);
    bool writeBrightness(int8_t value);
    bool writeContrast(int8_t value);
    bool writeSaturation(int8_t value);
    bool writeVerticalFlip(bool flip);
    bool writeHorizontalMirror(bool mirror);
    
    bool writePythonServerEnabled(bool enabled);
    bool writePythonServerIP(const char* ip, size_t length);
    bool writePythonServerPort(uint16_t port);
    
    bool writeDeviceName(const char* name, size_t length);
    bool writeMDNSHostname(const char* hostname, size_t length);
    
    // Debug
    void printSettings();
    
private:
    Preferences prefs;
    const char* NVS_NAMESPACE = "camera";
};

#endif // __CAMERA_SETTINGS_H__
```

### Step 1.2: Create `camera_settings.cpp`

**Location:** `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/v3_ino_2/camera_settings.cpp`

```cpp
#include "camera_settings.h"

CameraSettings::CameraSettings() {
    // Constructor
}

CameraSettings::~CameraSettings() {
    // Destructor
}

bool CameraSettings::isNVSInitialized() {
    prefs.begin(NVS_NAMESPACE, true); // Read-only
    bool initialized = prefs.getBool("initialized", false);
    prefs.end();
    return initialized;
}

bool CameraSettings::initializeNVS() {
    Serial.println("[SETTINGS] Initializing NVS with default values...");
    
    prefs.begin(NVS_NAMESPACE, false); // Read-write
    
    // Mark as initialized
    prefs.putBool("initialized", true);
    
    // Authentication
    prefs.putString("username", DefaultValues::USERNAME);
    prefs.putString("password", DefaultValues::PASSWORD);
    
    // Network
    prefs.putBool("useDHCP", DefaultValues::USE_DHCP);
    prefs.putBytes("staticIP", DefaultValues::STATIC_IP, 4);
    prefs.putBytes("gateway", DefaultValues::GATEWAY, 4);
    prefs.putBytes("subnet", DefaultValues::SUBNET, 4);
    
    // WiFi
    prefs.putString("wifiSSID", DefaultValues::WIFI_SSID);
    prefs.putString("wifiPass", DefaultValues::WIFI_PASSWORD);
    
    // Camera
    prefs.putUChar("camRes", DefaultValues::CAMERA_RESOLUTION);
    prefs.putUChar("camQual", DefaultValues::CAMERA_QUALITY);
    prefs.putUChar("frameRate", DefaultValues::FRAME_RATE);
    prefs.putChar("brightness", DefaultValues::BRIGHTNESS);
    prefs.putChar("contrast", DefaultValues::CONTRAST);
    prefs.putChar("saturation", DefaultValues::SATURATION);
    prefs.putBool("vFlip", DefaultValues::VERTICAL_FLIP);
    prefs.putBool("hMirror", DefaultValues::HORIZONTAL_MIRROR);
    
    // Python server
    prefs.putBool("pyEnabled", DefaultValues::PYTHON_SERVER_ENABLED);
    prefs.putString("pyIP", DefaultValues::PYTHON_SERVER_IP);
    prefs.putUShort("pyPort", DefaultValues::PYTHON_SERVER_PORT);
    
    // System
    prefs.putString("deviceName", DefaultValues::DEVICE_NAME);
    prefs.putString("mdnsHost", DefaultValues::MDNS_HOSTNAME);
    
    prefs.end();
    
    Serial.println("[SETTINGS] NVS initialized successfully");
    return true;
}

bool CameraSettings::resetToDefault() {
    Serial.println("[SETTINGS] Resetting to factory defaults...");
    
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear(); // Clear all keys in this namespace
    prefs.end();
    
    return initializeNVS();
}

void CameraSettings::readFromNVS() {
    prefs.begin(NVS_NAMESPACE, true); // Read-only
    
    // Authentication
    String user = prefs.getString("username", DefaultValues::USERNAME);
    String pass = prefs.getString("password", DefaultValues::PASSWORD);
    strncpy(username, user.c_str(), 31);
    strncpy(password, pass.c_str(), 31);
    username[31] = '\0';
    password[31] = '\0';
    
    // Network
    useDHCP = prefs.getBool("useDHCP", DefaultValues::USE_DHCP);
    prefs.getBytes("staticIP", staticIP, 4);
    prefs.getBytes("gateway", gateway, 4);
    prefs.getBytes("subnet", subnet, 4);
    
    // WiFi
    String ssid = prefs.getString("wifiSSID", DefaultValues::WIFI_SSID);
    String wifiPass = prefs.getString("wifiPass", DefaultValues::WIFI_PASSWORD);
    strncpy(wifiSSID, ssid.c_str(), 31);
    strncpy(wifiPassword, wifiPass.c_str(), 31);
    wifiSSID[31] = '\0';
    wifiPassword[31] = '\0';
    
    // Camera
    cameraResolution = prefs.getUChar("camRes", DefaultValues::CAMERA_RESOLUTION);
    cameraQuality = prefs.getUChar("camQual", DefaultValues::CAMERA_QUALITY);
    frameRate = prefs.getUChar("frameRate", DefaultValues::FRAME_RATE);
    brightness = prefs.getChar("brightness", DefaultValues::BRIGHTNESS);
    contrast = prefs.getChar("contrast", DefaultValues::CONTRAST);
    saturation = prefs.getChar("saturation", DefaultValues::SATURATION);
    verticalFlip = prefs.getBool("vFlip", DefaultValues::VERTICAL_FLIP);
    horizontalMirror = prefs.getBool("hMirror", DefaultValues::HORIZONTAL_MIRROR);
    
    // Python server
    pythonServerEnabled = prefs.getBool("pyEnabled", DefaultValues::PYTHON_SERVER_ENABLED);
    String pyIP = prefs.getString("pyIP", DefaultValues::PYTHON_SERVER_IP);
    strncpy(pythonServerIP, pyIP.c_str(), 31);
    pythonServerIP[31] = '\0';
    pythonServerPort = prefs.getUShort("pyPort", DefaultValues::PYTHON_SERVER_PORT);
    
    // System
    String devName = prefs.getString("deviceName", DefaultValues::DEVICE_NAME);
    String mdnsHost = prefs.getString("mdnsHost", DefaultValues::MDNS_HOSTNAME);
    strncpy(deviceName, devName.c_str(), 31);
    strncpy(mdnsHostname, mdnsHost.c_str(), 31);
    deviceName[31] = '\0';
    mdnsHostname[31] = '\0';
    
    prefs.end();
    
    Serial.println("[SETTINGS] Loaded from NVS");
}

// Write methods
bool CameraSettings::writeUsername(const char* user, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("username", user);
    prefs.end();
    strncpy(username, user, 31);
    username[31] = '\0';
    return true;
}

bool CameraSettings::writePassword(const char* pass, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("password", pass);
    prefs.end();
    strncpy(password, pass, 31);
    password[31] = '\0';
    return true;
}

bool CameraSettings::writeDHCPSetting(bool dhcp) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("useDHCP", dhcp);
    prefs.end();
    useDHCP = dhcp;
    return true;
}

bool CameraSettings::writeStaticIP(byte ip[4]) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBytes("staticIP", ip, 4);
    prefs.end();
    memcpy(staticIP, ip, 4);
    return true;
}

bool CameraSettings::writeGateway(byte gw[4]) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBytes("gateway", gw, 4);
    prefs.end();
    memcpy(gateway, gw, 4);
    return true;
}

bool CameraSettings::writeSubnet(byte sn[4]) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBytes("subnet", sn, 4);
    prefs.end();
    memcpy(subnet, sn, 4);
    return true;
}

bool CameraSettings::writeSSID(const char* ssid, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("wifiSSID", ssid);
    prefs.end();
    strncpy(wifiSSID, ssid, 31);
    wifiSSID[31] = '\0';
    return true;
}

bool CameraSettings::writeWiFiPassword(const char* pass, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("wifiPass", pass);
    prefs.end();
    strncpy(wifiPassword, pass, 31);
    wifiPassword[31] = '\0';
    return true;
}

bool CameraSettings::writeCameraResolution(uint8_t resolution) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar("camRes", resolution);
    prefs.end();
    cameraResolution = resolution;
    return true;
}

bool CameraSettings::writeCameraQuality(uint8_t quality) {
    if (quality < 10 || quality > 63) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar("camQual", quality);
    prefs.end();
    cameraQuality = quality;
    return true;
}

bool CameraSettings::writeFrameRate(uint8_t fps) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar("frameRate", fps);
    prefs.end();
    frameRate = fps;
    return true;
}

bool CameraSettings::writeBrightness(int8_t value) {
    if (value < -2 || value > 2) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putChar("brightness", value);
    prefs.end();
    brightness = value;
    return true;
}

bool CameraSettings::writeContrast(int8_t value) {
    if (value < -2 || value > 2) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putChar("contrast", value);
    prefs.end();
    contrast = value;
    return true;
}

bool CameraSettings::writeSaturation(int8_t value) {
    if (value < -2 || value > 2) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putChar("saturation", value);
    prefs.end();
    saturation = value;
    return true;
}

bool CameraSettings::writeVerticalFlip(bool flip) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("vFlip", flip);
    prefs.end();
    verticalFlip = flip;
    return true;
}

bool CameraSettings::writeHorizontalMirror(bool mirror) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("hMirror", mirror);
    prefs.end();
    horizontalMirror = mirror;
    return true;
}

bool CameraSettings::writePythonServerEnabled(bool enabled) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("pyEnabled", enabled);
    prefs.end();
    pythonServerEnabled = enabled;
    return true;
}

bool CameraSettings::writePythonServerIP(const char* ip, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("pyIP", ip);
    prefs.end();
    strncpy(pythonServerIP, ip, 31);
    pythonServerIP[31] = '\0';
    return true;
}

bool CameraSettings::writePythonServerPort(uint16_t port) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUShort("pyPort", port);
    prefs.end();
    pythonServerPort = port;
    return true;
}

bool CameraSettings::writeDeviceName(const char* name, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("deviceName", name);
    prefs.end();
    strncpy(deviceName, name, 31);
    deviceName[31] = '\0';
    return true;
}

bool CameraSettings::writeMDNSHostname(const char* hostname, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("mdnsHost", hostname);
    prefs.end();
    strncpy(mdnsHostname, hostname, 31);
    mdnsHostname[31] = '\0';
    return true;
}

void CameraSettings::printSettings() {
    Serial.println("\n========== Camera Settings ==========");
    Serial.printf("Username: %s\n", username);
    Serial.println("Password: [HIDDEN]");
    Serial.printf("WiFi SSID: %s\n", wifiSSID);
    Serial.println("WiFi Password: [HIDDEN]");
    Serial.printf("Use DHCP: %s\n", useDHCP ? "Yes" : "No");
    Serial.printf("Static IP: %d.%d.%d.%d\n", staticIP[0], staticIP[1], staticIP[2], staticIP[3]);
    Serial.printf("Gateway: %d.%d.%d.%d\n", gateway[0], gateway[1], gateway[2], gateway[3]);
    Serial.printf("Subnet: %d.%d.%d.%d\n", subnet[0], subnet[1], subnet[2], subnet[3]);
    Serial.printf("Camera Resolution: %d\n", cameraResolution);
    Serial.printf("Camera Quality: %d\n", cameraQuality);
    Serial.printf("Frame Rate: %d fps\n", frameRate);
    Serial.printf("Brightness: %d\n", brightness);
    Serial.printf("Contrast: %d\n", contrast);
    Serial.printf("Saturation: %d\n", saturation);
    Serial.printf("Vertical Flip: %s\n", verticalFlip ? "Yes" : "No");
    Serial.printf("Horizontal Mirror: %s\n", horizontalMirror ? "Yes" : "No");
    Serial.printf("Python Server Enabled: %s\n", pythonServerEnabled ? "Yes" : "No");
    Serial.printf("Python Server: %s:%d\n", pythonServerIP, pythonServerPort);
    Serial.printf("Device Name: %s\n", deviceName);
    Serial.printf("mDNS Hostname: %s\n", mdnsHostname);
    Serial.println("=====================================\n");
}
```

### Step 1.3: Test Settings Persistence

Add to main `.ino` file temporarily:

```cpp
#include "camera_settings.h"

CameraSettings* settings;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    settings = new CameraSettings();
    
    // Check if NVS is initialized
    if (!settings->isNVSInitialized()) {
        Serial.println("First boot - initializing NVS...");
        settings->initializeNVS();
    }
    
    // Load settings
    settings->readFromNVS();
    settings->printSettings();
    
    // Test: Change a setting and restart to verify persistence
    settings->writeDeviceName("TestCamera", 10);
    Serial.println("Changed device name to 'TestCamera' - restart to verify persistence");
}

void loop() {
    delay(1000);
}
```

**Test Process:**
1. Upload code
2. Check serial output - should show default settings
3. Restart ESP32
4. Check serial output - device name should be "TestCamera"
5. ✅ Settings persist correctly

---

## Phase 2: Web Server + Authentication

### Step 2.1: Install Required Libraries

```
Arduino IDE → Library Manager:
1. ESPAsyncWebServer by me-no-dev
2. AsyncTCP by me-no-dev
```

### Step 2.2: Create `web_server.h` (Part 1: Basic Structure)

**Location:** `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/v3_ino_2/web_server.h`

```cpp
#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "camera_settings.h"

class CameraWebServer {
public:
    CameraWebServer(uint16_t port, CameraSettings* settings);
    ~CameraWebServer();
    
    void begin();
    void loop(); // For any periodic tasks
    
    // Callbacks
    void setApplyCameraSettingsCallback(std::function<bool()> callback);
    
private:
    AsyncWebServer* server;
    CameraSettings* settings_;
    
    // Session management
    String authToken_;
    unsigned long lastActivityTime_;
    const unsigned long COOKIE_TIMEOUT = 1800000; // 30 minutes
    
    // Camera settings callback
    std::function<bool()> applyCameraSettingsCallback_;
    
    // Helper methods
    String generateRandomToken();
    bool isAuthenticated(AsyncWebServerRequest *request);
    bool parseIPAddress(const String& ipStr, byte* ip);
    
    // Route handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleLogin(AsyncWebServerRequest *request);
    void handleLogout(AsyncWebServerRequest *request);
    void handleChangePassword(AsyncWebServerRequest *request);
    void handleGetSettings(AsyncWebServerRequest *request);
    void handlePostSettings(AsyncWebServerRequest *request);
    void handleGetStatus(AsyncWebServerRequest *request);
    void handleCameraConfig(AsyncWebServerRequest *request);
    void handleStream(AsyncWebServerRequest *request);
    void handleCapture(AsyncWebServerRequest *request);
    void handle404(AsyncWebServerRequest *request);
    
    // HTML pages
    void sendLoginPage(AsyncWebServerRequest *request);
    void sendMainPage(AsyncWebServerRequest *request);
    
    // Response helpers
    void send401Unauthorized(AsyncWebServerRequest *request);
    void sendJsonResponse(AsyncWebServerRequest *request, int code, const char* status, const char* message);
};

#endif // __WEB_SERVER_H__
```

### Step 2.3: Create `web_server.cpp` (Part 1: Constructor and Basic Auth)

**Location:** `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/v3_ino_2/web_server.cpp`

```cpp
#include "web_server.h"

CameraWebServer::CameraWebServer(uint16_t port, CameraSettings* settings)
    : settings_(settings), lastActivityTime_(0) {
    server = new AsyncWebServer(port);
}

CameraWebServer::~CameraWebServer() {
    delete server;
}

void CameraWebServer::begin() {
    // Root
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });
    
    // Authentication endpoints
    server->on("/api/login", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleLogin(request);
    });
    
    server->on("/api/logout", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleLogout(request);
    });
    
    server->on("/api/change-password", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleChangePassword(request);
    });
    
    // Settings endpoints
    server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleGetSettings(request);
    });
    
    server->on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handlePostSettings(request);
    });
    
    server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleGetStatus(request);
    });
    
    // Camera endpoints
    server->on("/api/camera/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleCameraConfig(request);
    });
    
    server->on("/stream", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStream(request);
    });
    
    server->on("/capture", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleCapture(request);
    });
    
    // 404 handler
    server->onNotFound([this](AsyncWebServerRequest *request) {
        handle404(request);
    });
    
    server->begin();
    Serial.println("[WEB] Server started on port 80");
}

void CameraWebServer::loop() {
    // Check for session timeout
    if (authToken_.length() > 0 && (millis() - lastActivityTime_) > COOKIE_TIMEOUT) {
        Serial.println("[WEB] Session expired");
        authToken_ = "";
    }
}

void CameraWebServer::setApplyCameraSettingsCallback(std::function<bool()> callback) {
    applyCameraSettingsCallback_ = callback;
}

String CameraWebServer::generateRandomToken() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int tokenLength = 32;
    String token = "";
    
    for (int i = 0; i < tokenLength; i++) {
        int index = random(0, sizeof(charset) - 1);
        token += charset[index];
    }
    return token;
}

bool CameraWebServer::isAuthenticated(AsyncWebServerRequest *request) {
    if (request->hasHeader("Cookie")) {
        String cookie = request->header("Cookie");
        int tokenIndex = cookie.indexOf("auth_token=");
        if (tokenIndex != -1) {
            int tokenEnd = cookie.indexOf(";", tokenIndex);
            String token = (tokenEnd == -1) ? 
                cookie.substring(tokenIndex + 11) : 
                cookie.substring(tokenIndex + 11, tokenEnd);
            
            if (token == authToken_ && authToken_ != "" && 
                (millis() - lastActivityTime_) < COOKIE_TIMEOUT) {
                lastActivityTime_ = millis();
                return true;
            }
        }
    }
    return false;
}

void CameraWebServer::send401Unauthorized(AsyncWebServerRequest *request) {
    sendJsonResponse(request, 401, "error", "Unauthorized");
}

void CameraWebServer::sendJsonResponse(AsyncWebServerRequest *request, int code, 
                                       const char* status, const char* message) {
    StaticJsonDocument<200> doc;
    doc["status"] = status;
    if (message) doc["message"] = message;
    
    String response;
    serializeJson(doc, response);
    request->send(code, "application/json", response);
}

// Root handler
void CameraWebServer::handleRoot(AsyncWebServerRequest *request) {
    if (isAuthenticated(request)) {
        sendMainPage(request);
    } else {
        sendLoginPage(request);
    }
}

// Login handler
void CameraWebServer::handleLogin(AsyncWebServerRequest *request) {
    if (!request->hasArg("username") || !request->hasArg("password")) {
        sendJsonResponse(request, 400, "error", "Missing username or password");
        return;
    }
    
    String username = request->arg("username");
    String password = request->arg("password");
    
    if (username == settings_->username && password == settings_->password) {
        authToken_ = generateRandomToken();
        lastActivityTime_ = millis();
        
        StaticJsonDocument<200> doc;
        doc["status"] = "success";
        doc["message"] = "Login successful";
        doc["token"] = authToken_;
        doc["expires_in"] = COOKIE_TIMEOUT / 1000;
        
        String responseBody;
        serializeJson(doc, responseBody);
        
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseBody);
        String cookie = "auth_token=" + authToken_ + "; Max-Age=" + 
                       String(COOKIE_TIMEOUT / 1000) + "; Path=/; HttpOnly";
        response->addHeader("Set-Cookie", cookie);
        request->send(response);
        
        Serial.printf("[WEB] User '%s' logged in\n", username.c_str());
    } else {
        sendJsonResponse(request, 401, "error", "Invalid username or password");
    }
}

// Logout handler
void CameraWebServer::handleLogout(AsyncWebServerRequest *request) {
    authToken_ = "";
    lastActivityTime_ = 0;
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", 
                                                              "{\"status\":\"success\",\"message\":\"Logged out\"}");
    String cookie = "auth_token=; Max-Age=0; Path=/; HttpOnly";
    response->addHeader("Set-Cookie", cookie);
    request->send(response);
    
    Serial.println("[WEB] User logged out");
}

// Change password handler
void CameraWebServer::handleChangePassword(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        send401Unauthorized(request);
        return;
    }
    
    if (!request->hasArg("current_password") || !request->hasArg("new_password")) {
        sendJsonResponse(request, 400, "error", "Missing current or new password");
        return;
    }
    
    String currentPassword = request->arg("current_password");
    String newPassword = request->arg("new_password");
    
    if (currentPassword != String(settings_->password)) {
        sendJsonResponse(request, 401, "error", "Current password is incorrect");
        return;
    }
    
    if (newPassword.length() < 4 || newPassword.length() > 31) {
        sendJsonResponse(request, 400, "error", "Password must be 4-31 characters");
        return;
    }
    
    if (settings_->writePassword(newPassword.c_str(), newPassword.length())) {
        settings_->readFromNVS(); // Reload to update in-memory copy
        sendJsonResponse(request, 200, "success", "Password changed successfully");
        Serial.println("[WEB] Password changed");
    } else {
        sendJsonResponse(request, 500, "error", "Failed to save password");
    }
}

void CameraWebServer::handle404(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404 Not Found");
}
```

### Step 2.4: HTML Pages (Login)

Add to `web_server.cpp`:

```cpp
void CameraWebServer::sendLoginPage(AsyncWebServerRequest *request) {
    const char* html = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Camera Login</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #0B0E14;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .login-container {
            background: #1E293B;
            padding: 40px;
            border-radius: 12px;
            border: 1px solid #334155;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
            width: 100%;
            max-width: 400px;
        }
        h1 {
            text-align: center;
            color: #F8FAFC;
            margin-bottom: 30px;
            font-size: 24px;
            font-weight: 600;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #94A3B8;
            font-size: 14px;
            font-weight: 500;
        }
        input[type="text"],
        input[type="password"] {
            width: 100%;
            padding: 12px;
            border: 1px solid #334155;
            border-radius: 8px;
            font-size: 14px;
            background: #0F172A;
            color: #F8FAFC;
            transition: border-color 0.2s;
        }
        input:focus {
            outline: none;
            border-color: #38BDF8;
        }
        .btn-login {
            width: 100%;
            padding: 12px;
            background: #38BDF8;
            color: #0F172A;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-login:hover {
            background: #22D3EE;
            transform: translateY(-1px);
        }
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 20px;
            text-align: center;
            display: none;
        }
        .message.error {
            background: #7F1D1D;
            color: #FCA5A5;
            border: 1px solid #991B1B;
            display: block;
        }
        .lock-icon {
            text-align: center;
            font-size: 48px;
            margin-bottom: 20px;
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="lock-icon">🔒</div>
        <h1>Camera Login</h1>
        <div id="message" class="message"></div>
        <form id="loginForm">
            <div class="form-group">
                <label for="username">Username:</label>
                <input type="text" id="username" name="username" required autofocus>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit" class="btn-login">Login</button>
        </form>
    </div>
    <script>
        document.getElementById('loginForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;
            const messageDiv = document.getElementById('message');
            
            try {
                const response = await fetch('/api/login', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
                });
                
                const data = await response.json();
                
                if (response.ok && data.status === 'success') {
                    window.location.href = '/';
                } else {
                    messageDiv.className = 'message error';
                    messageDiv.textContent = data.message || 'Invalid credentials';
                }
            } catch (error) {
                messageDiv.className = 'message error';
                messageDiv.textContent = 'Login failed. Please try again.';
            }
        });
    </script>
</body>
</html>
)HTML";
    
    request->send(200, "text/html", html);
}
```

**Continue with remaining handlers and main page HTML in next steps...**

---

## Phase 3-9: Detailed Implementation Steps

**Note for AI Assistant:** Due to length constraints, this guide covers Phase 1-2 in detail. For subsequent phases:

### **Phase 3: Settings Management API**
- Reference Gate's `handleGetSettings()` and `handlePostSettings()` in `/Users/szemy/Workspace/Gate/Gate_V1.0/api_server.h` lines 496-646
- Adapt for camera settings structure
- Add WiFi reconnection logic after settings change

### **Phase 4: MJPEG Streaming**
- Implement chunked response streaming
- Use `request->beginChunkedResponse()` from AsyncWebServer
- Format: `--frame\r\nContent-Type: image/jpeg\r\n\r\n[data]\r\n`
- Store latest frame in global variable with mutex protection

### **Phase 5: Camera Settings UI**
- Add camera controls to main HTML page
- Add live preview: `<img src="/stream">`
- Reference Gate's settings form structure (lines 968-1048 in api_server.h)
- Adapt styling to match login page (dark theme with #0B0E14 background)

### **Phase 6: Dual-Core Architecture**
- Create `cameraTask()` for Core 0
- Create `networkTask()` for Core 1
- Use `SemaphoreHandle_t` for frame buffer protection
- Use `xTaskCreatePinnedToCore()` in setup()
- Empty main `loop()` function

### **Phase 7: Python Server Integration**
- Add polling loop in `networkTask()`
- HTTP GET to Python server's `/commands` endpoint every 2s
- Parse JSON response for commands
- Execute commands (LED flash, resolution change, etc.)

### **Phase 8: Additional Features**
- Add `#include <ESPmDNS.h>`
- Call `MDNS.begin(settings->mdnsHostname)` after WiFi connects
- Add ArduinoOTA with password from settings
- Add LED status indicators based on current v3_ino_2.ino pattern

### **Phase 9: Testing**
- Test all endpoints with Postman or curl
- Test multiple browser connections
- Monitor serial output for errors
- Test settings persistence with power cycles
- Create USER_GUIDE.md with screenshots

---

## 🔧 Key Integration Points

### Main .ino File Structure

```cpp
#include "camera_settings.h"
#include "web_server.h"
#include "esp_camera.h"
#include <WiFi.h>

CameraSettings* settings;
CameraWebServer* webServer;

TaskHandle_t CameraTask;
TaskHandle_t NetworkTask;

SemaphoreHandle_t frameMutex;
camera_fb_t* latestFrame = nullptr;

void setup() {
    Serial.begin(115200);
    
    // Initialize settings
    settings = new CameraSettings();
    if (!settings->isNVSInitialized()) {
        settings->initializeNVS();
    }
    settings->readFromNVS();
    settings->printSettings();
    
    // Initialize camera
    initCamera();
    
    // Initialize mutex
    frameMutex = xSemaphoreCreateMutex();
    
    // Connect WiFi
    connectWiFi();
    
    // Create tasks
    xTaskCreatePinnedToCore(cameraTask, "Camera", 8192, NULL, 2, &CameraTask, 0);
    xTaskCreatePinnedToCore(networkTask, "Network", 10000, NULL, 1, &NetworkTask, 1);
}

void loop() {
    // Empty - all work in tasks
}

void cameraTask(void* param) {
    while (true) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            xSemaphoreTake(frameMutex, portMAX_DELAY);
            if (latestFrame) esp_camera_fb_return(latestFrame);
            latestFrame = fb;
            xSemaphoreGive(frameMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000 / settings->frameRate));
    }
}

void networkTask(void* param) {
    webServer = new CameraWebServer(80, settings);
    webServer->begin();
    
    while (true) {
        webServer->loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

---

## 📝 Testing Checklist

After each phase:
- [ ] Code compiles without errors
- [ ] Upload to ESP32-S3
- [ ] Check serial output for errors
- [ ] Test new functionality
- [ ] Verify settings persist across reboot
- [ ] Commit to git with clear message

---

## 🚨 Common Issues & Solutions

### Issue: AsyncWebServer compile errors
**Solution:** Install correct version:
- ESPAsyncWebServer by me-no-dev (1.2.3+)
- AsyncTCP by me-no-dev (1.1.1+)

### Issue: Camera initialization fails
**Solution:** Check pin definitions match your hardware (lines 29-45 in current v3_ino_2.ino)

### Issue: Settings don't persist
**Solution:** 
- Check `prefs.begin()` and `prefs.end()` are called
- Verify NVS partition in partition table
- Try `prefs.clear()` to reset

### Issue: MJPEG stream doesn't display
**Solution:**
- Check Content-Type is `multipart/x-mixed-replace; boundary=frame`
- Verify frame boundaries are correct
- Test with VLC player first (more forgiving than browsers)

### Issue: Web server crashes under load
**Solution:**
- Increase task stack size (8192 → 10000)
- Add mutex protection to all shared resources
- Check for memory leaks with `ESP.getFreeHeap()`

---

## 📚 Reference Files

Always refer to these for patterns:
- **Gate main:** `/Users/szemy/Workspace/Gate/Gate_V1.0/Gate_V1.0.ino`
- **Gate settings:** `/Users/szemy/Workspace/Gate/Gate_V1.0/setting.h` and `.cpp`
- **Gate web server:** `/Users/szemy/Workspace/Gate/Gate_V1.0/api_server.h`
- **Current camera:** `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/v3_ino_2/v3_ino_2.ino`

---

## ✅ Next Steps

1. Start with Phase 1 (Storage Migration)
2. Test thoroughly before moving to Phase 2
3. Follow the detailed code examples provided
4. Reference Gate files for any unclear patterns
5. Ask for clarification if implementation details are ambiguous

**Good luck with the implementation!**
