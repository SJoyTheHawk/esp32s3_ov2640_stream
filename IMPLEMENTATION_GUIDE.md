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
        // Authentication - Admin (full access)
        static constexpr char ADMIN_USERNAME[32] = "admin";
        static constexpr char ADMIN_PASSWORD[32] = "admin";
        
        // Authentication - User (camera access only)
        static constexpr char USER_USERNAME[32] = "user";
        static constexpr char USER_PASSWORD[32] = "user";
        
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
    
    // Authentication - Admin (full access)
    char adminUsername[32];
    char adminPassword[32];
    
    // Authentication - User (camera access only)
    char userUsername[32];
    char userPassword[32];
    
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
    bool writeAdminUsername(const char* user, size_t length);
    bool writeAdminPassword(const char* pass, size_t length);
    bool writeUserUsername(const char* user, size_t length);
    bool writeUserPassword(const char* pass, size_t length);
    
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
    
    // Authentication - Admin
    prefs.putString("adminUser", DefaultValues::ADMIN_USERNAME);
    prefs.putString("adminPass", DefaultValues::ADMIN_PASSWORD);
    
    // Authentication - User
    prefs.putString("userUser", DefaultValues::USER_USERNAME);
    prefs.putString("userPass", DefaultValues::USER_PASSWORD);
    
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
    
    // Authentication - Admin
    String adminUser = prefs.getString("adminUser", DefaultValues::ADMIN_USERNAME);
    String adminPass = prefs.getString("adminPass", DefaultValues::ADMIN_PASSWORD);
    strncpy(adminUsername, adminUser.c_str(), 31);
    strncpy(adminPassword, adminPass.c_str(), 31);
    adminUsername[31] = '\0';
    adminPassword[31] = '\0';
    
    // Authentication - User
    String user = prefs.getString("userUser", DefaultValues::USER_USERNAME);
    String pass = prefs.getString("userPass", DefaultValues::USER_PASSWORD);
    strncpy(userUsername, user.c_str(), 31);
    strncpy(userPassword, pass.c_str(), 31);
    userUsername[31] = '\0';
    userPassword[31] = '\0';
    
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
bool CameraSettings::writeAdminUsername(const char* user, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("adminUser", user);
    prefs.end();
    strncpy(adminUsername, user, 31);
    adminUsername[31] = '\0';
    return true;
}

bool CameraSettings::writeAdminPassword(const char* pass, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("adminPass", pass);
    prefs.end();
    strncpy(adminPassword, pass, 31);
    adminPassword[31] = '\0';
    return true;
}

bool CameraSettings::writeUserUsername(const char* user, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("userUser", user);
    prefs.end();
    strncpy(userUsername, user, 31);
    userUsername[31] = '\0';
    return true;
}

bool CameraSettings::writeUserPassword(const char* pass, size_t length) {
    if (length > 31) return false;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("userPass", pass);
    prefs.end();
    strncpy(userPassword, pass, 31);
    userPassword[31] = '\0';
    return true;
}

// Network write methods...
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
    String adminAuthToken_;
    String userAuthToken_;
    unsigned long adminLastActivityTime_;
    unsigned long userLastActivityTime_;
    const unsigned long COOKIE_TIMEOUT = 1800000; // 30 minutes
    
    enum class AuthLevel {
        NONE,
        USER,    // Camera access only
        ADMIN    // Full access
    };
    
    // Camera settings callback
    std::function<bool()> applyCameraSettingsCallback_;
    
    // Helper methods
    String generateRandomToken();
    AuthLevel getAuthLevel(AsyncWebServerRequest *request);
    bool isAuthenticated(AsyncWebServerRequest *request);  // Any valid login
    bool isAdminAuthenticated(AsyncWebServerRequest *request);  // Admin only
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
    : settings_(settings), adminLastActivityTime_(0), userLastActivityTime_(0) {
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
    // Check for admin session timeout
    if (adminAuthToken_.length() > 0 && (millis() - adminLastActivityTime_) > COOKIE_TIMEOUT) {
        Serial.println("[WEB] Admin session expired");
        adminAuthToken_ = "";
    }
    
    // Check for user session timeout
    if (userAuthToken_.length() > 0 && (millis() - userLastActivityTime_) > COOKIE_TIMEOUT) {
        Serial.println("[WEB] User session expired");
        userAuthToken_ = "";
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

CameraWebServer::AuthLevel CameraWebServer::getAuthLevel(AsyncWebServerRequest *request) {
    if (request->hasHeader("Cookie")) {
        String cookie = request->header("Cookie");
        int tokenIndex = cookie.indexOf("auth_token=");
        if (tokenIndex != -1) {
            int tokenEnd = cookie.indexOf(";", tokenIndex);
            String token = (tokenEnd == -1) ? 
                cookie.substring(tokenIndex + 11) : 
                cookie.substring(tokenIndex + 11, tokenEnd);
            
            // Check admin token
            if (token == adminAuthToken_ && adminAuthToken_ != "" && 
                (millis() - adminLastActivityTime_) < COOKIE_TIMEOUT) {
                adminLastActivityTime_ = millis();
                return AuthLevel::ADMIN;
            }
            
            // Check user token
            if (token == userAuthToken_ && userAuthToken_ != "" && 
                (millis() - userLastActivityTime_) < COOKIE_TIMEOUT) {
                userLastActivityTime_ = millis();
                return AuthLevel::USER;
            }
        }
    }
    return AuthLevel::NONE;
}

bool CameraWebServer::isAuthenticated(AsyncWebServerRequest *request) {
    return getAuthLevel(request) != AuthLevel::NONE;
}

bool CameraWebServer::isAdminAuthenticated(AsyncWebServerRequest *request) {
    return getAuthLevel(request) == AuthLevel::ADMIN;
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
    String token = "";
    String userRole = "";
    unsigned long* lastActivity = nullptr;
    String* authToken = nullptr;
    
    // Check admin credentials
    if (username == settings_->adminUsername && password == settings_->adminPassword) {
        adminAuthToken_ = generateRandomToken();
        adminLastActivityTime_ = millis();
        token = adminAuthToken_;
        userRole = "admin";
        Serial.printf("[WEB] Admin '%s' logged in\n", username.c_str());
    }
    // Check user credentials
    else if (username == settings_->userUsername && password == settings_->userPassword) {
        userAuthToken_ = generateRandomToken();
        userLastActivityTime_ = millis();
        token = userAuthToken_;
        userRole = "user";
        Serial.printf("[WEB] User '%s' logged in\n", username.c_str());
    }
    // Invalid credentials
    else {
        sendJsonResponse(request, 401, "error", "Invalid username or password");
        return;
    }
    
    // Send success response with token
    StaticJsonDocument<200> doc;
    doc["status"] = "success";
    doc["message"] = "Login successful";
    doc["token"] = token;
    doc["role"] = userRole;
    doc["expires_in"] = COOKIE_TIMEOUT / 1000;
    
    String responseBody;
    serializeJson(doc, responseBody);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseBody);
    String cookie = "auth_token=" + token + "; Max-Age=" + 
                   String(COOKIE_TIMEOUT / 1000) + "; Path=/; HttpOnly";
    response->addHeader("Set-Cookie", cookie);
    request->send(response);
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
    AuthLevel authLevel = getAuthLevel(request);
    if (authLevel == AuthLevel::NONE) {
        send401Unauthorized(request);
        return;
    }
    
    if (!request->hasArg("current_password") || !request->hasArg("new_password") || 
        !request->hasArg("confirm_password")) {
        sendJsonResponse(request, 400, "error", "Missing required fields");
        return;
    }
    
    String currentPassword = request->arg("current_password");
    String newPassword = request->arg("new_password");
    String confirmPassword = request->arg("confirm_password");
    String targetUser = request->hasArg("targetUser") ? request->arg("targetUser") : "";
    
    // Verify confirm password matches
    if (newPassword != confirmPassword) {
        sendJsonResponse(request, 400, "error", "New passwords do not match");
        return;
    }
    
    // Users can only change their own password
    if (authLevel == AuthLevel::USER) {
        if (targetUser != "" && targetUser != "user") {
            sendJsonResponse(request, 403, "error", "Users can only change their own password");
            return;
        }
        
        // Verify current password
        if (currentPassword != String(settings_->userPassword)) {
            sendJsonResponse(request, 401, "error", "Current password is incorrect");
            return;
        }
        
        if (newPassword.length() < 4 || newPassword.length() > 31) {
            sendJsonResponse(request, 400, "error", "Password must be 4-31 characters");
            return;
        }
        
        settings_->writeUserPassword(newPassword.c_str(), newPassword.length());
        sendJsonResponse(request, 200, "success", "Password changed successfully");
        return;
    }
    
    // Admins can change any password
    if (authLevel == AuthLevel::ADMIN) {
        if (newPassword.length() < 4 || newPassword.length() > 31) {
            sendJsonResponse(request, 400, "error", "Password must be 4-31 characters");
            return;
        }
        
        if (targetUser == "user") {
            // Verify admin's current password
            if (currentPassword != String(settings_->adminPassword)) {
                sendJsonResponse(request, 401, "error", "Current admin password is incorrect");
                return;
            }
            settings_->writeUserPassword(newPassword.c_str(), newPassword.length());
        } else {
            // Changing own (admin) password
            if (currentPassword != String(settings_->adminPassword)) {
                sendJsonResponse(request, 401, "error", "Current password is incorrect");
                return;
            }
            settings_->writeAdminPassword(newPassword.c_str(), newPassword.length());
        }
        
        sendJsonResponse(request, 200, "success", "Password changed successfully");
        Serial.println("[WEB] Password changed");
        return;
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

### Step 2.5: Main Page HTML with Modal Dialogs

Add to `web_server.cpp`:

```cpp
void CameraWebServer::sendMainPage(AsyncWebServerRequest *request) {
    const char* html = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Camera Control</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #0B0E14;
            color: #F8FAFC;
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .header {
            background: #1E293B;
            padding: 20px;
            border-radius: 12px;
            border: 1px solid #334155;
            margin-bottom: 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        h1 {
            font-size: 24px;
            font-weight: 600;
        }
        .header-actions {
            display: flex;
            gap: 10px;
            align-items: center;
        }
        .user-info {
            color: #94A3B8;
            margin-right: 15px;
            font-size: 14px;
        }
        .btn {
            padding: 10px 20px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-primary {
            background: #38BDF8;
            color: #0F172A;
        }
        .btn-primary:hover {
            background: #22D3EE;
            transform: translateY(-1px);
        }
        .btn-secondary {
            background: #475569;
            color: #F8FAFC;
        }
        .btn-secondary:hover {
            background: #64748B;
        }
        .btn-logout {
            background: #DC2626;
            color: #FFF;
            padding: 12px 24px;
            font-size: 16px;
            font-weight: 600;
        }
        .btn-logout:hover {
            background: #B91C1C;
        }
        .main-content {
            display: grid;
            grid-template-columns: 2fr 1fr;
            gap: 20px;
        }
        .panel {
            background: #1E293B;
            padding: 24px;
            border-radius: 12px;
            border: 1px solid #334155;
        }
        .panel h2 {
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 20px;
            color: #38BDF8;
        }
        .stream-container {
            width: 100%;
            background: #000;
            border-radius: 8px;
            overflow: hidden;
            min-height: 480px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .stream-container img {
            width: 100%;
            height: auto;
            display: block;
        }
        .settings-group {
            margin-bottom: 24px;
        }
        .settings-group:last-child {
            margin-bottom: 0;
        }
        .settings-group h3 {
            font-size: 14px;
            color: #94A3B8;
            margin-bottom: 12px;
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }
        .setting-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 0;
            border-bottom: 1px solid #334155;
        }
        .setting-item:last-child {
            border-bottom: none;
        }
        .setting-label {
            color: #CBD5E1;
            font-size: 14px;
        }
        .setting-value {
            color: #38BDF8;
            font-weight: 500;
        }
        
        /* Modal Styles */
        .modal {
            display: none;
            position: fixed;
            z-index: 1000;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0, 0, 0, 0.7);
            animation: fadeIn 0.2s;
        }
        .modal.show {
            display: flex;
            align-items: center;
            justify-content: center;
        }
        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }
        .modal-content {
            background: #1E293B;
            border: 1px solid #334155;
            border-radius: 12px;
            padding: 30px;
            width: 90%;
            max-width: 500px;
            max-height: 90vh;
            overflow-y: auto;
            animation: slideDown 0.3s;
        }
        @keyframes slideDown {
            from {
                transform: translateY(-50px);
                opacity: 0;
            }
            to {
                transform: translateY(0);
                opacity: 1;
            }
        }
        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 24px;
        }
        .modal-header h2 {
            font-size: 20px;
            color: #F8FAFC;
        }
        .close-btn {
            background: none;
            border: none;
            color: #94A3B8;
            font-size: 28px;
            cursor: pointer;
            padding: 0;
            width: 30px;
            height: 30px;
            line-height: 1;
        }
        .close-btn:hover {
            color: #F8FAFC;
        }
        .form-group {
            margin-bottom: 20px;
        }
        .form-group label {
            display: block;
            margin-bottom: 8px;
            color: #94A3B8;
            font-size: 14px;
            font-weight: 500;
        }
        .form-group input[type="text"],
        .form-group input[type="password"] {
            width: 100%;
            padding: 12px;
            border: 1px solid #334155;
            border-radius: 8px;
            font-size: 14px;
            background: #0F172A;
            color: #F8FAFC;
            transition: border-color 0.2s;
        }
        .form-group input:focus {
            outline: none;
            border-color: #38BDF8;
        }
        .form-actions {
            display: flex;
            gap: 10px;
            justify-content: flex-end;
            margin-top: 24px;
        }
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 20px;
            text-align: center;
            display: none;
        }
        .message.success {
            background: #065F46;
            color: #6EE7B7;
            border: 1px solid #047857;
            display: block;
        }
        .message.error {
            background: #7F1D1D;
            color: #FCA5A5;
            border: 1px solid #991B1B;
            display: block;
        }
        .admin-only {
            display: none;
        }
        
        /* Camera Controls */
        .camera-controls {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-top: 20px;
        }
        .camera-control {
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        .camera-control label {
            color: #94A3B8;
            font-size: 13px;
        }
        .camera-control select,
        .camera-control input {
            padding: 8px;
            border: 1px solid #334155;
            border-radius: 6px;
            background: #0F172A;
            color: #F8FAFC;
            font-size: 13px;
        }
        
        @media (max-width: 768px) {
            .main-content {
                grid-template-columns: 1fr;
            }
            .header {
                flex-direction: column;
                gap: 15px;
            }
            .header-actions {
                width: 100%;
                flex-wrap: wrap;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header">
            <h1>📷 ESP32 Camera Control</h1>
            <div class="header-actions">
                <span class="user-info" id="userInfo">Loading...</span>
                <button class="btn btn-logout" onclick="logout()">Logout</button>
            </div>
        </div>
        
        <!-- Main Content -->
        <div class="main-content">
            <!-- Left Column: Stream -->
            <div class="panel">
                <h2>Live Stream</h2>
                <div class="stream-container">
                    <img id="stream" src="/stream" alt="Camera Stream">
                </div>
                
                <!-- Camera Controls -->
                <div class="camera-controls">
                    <div class="camera-control">
                        <label>Resolution</label>
                        <select id="resolution" onchange="updateCamera()">
                            <option value="8">QVGA (320x240)</option>
                            <option value="9">VGA (640x480)</option>
                            <option value="10" selected>SVGA (800x600)</option>
                            <option value="11">XGA (1024x768)</option>
                            <option value="12">HD (1280x720)</option>
                        </select>
                    </div>
                    <div class="camera-control">
                        <label>Quality (10-63)</label>
                        <input type="number" id="quality" value="12" min="10" max="63" onchange="updateCamera()">
                    </div>
                    <div class="camera-control">
                        <label>Brightness (-2 to 2)</label>
                        <input type="number" id="brightness" value="0" min="-2" max="2" onchange="updateCamera()">
                    </div>
                    <div class="camera-control">
                        <label>Contrast (-2 to 2)</label>
                        <input type="number" id="contrast" value="0" min="-2" max="2" onchange="updateCamera()">
                    </div>
                </div>
            </div>
            
            <!-- Right Column: Settings -->
            <div class="panel">
                <h2>System Settings</h2>
                
                <div class="settings-group">
                    <h3>Network</h3>
                    <div class="setting-item">
                        <span class="setting-label">IP Address</span>
                        <span class="setting-value" id="ipAddress">Loading...</span>
                    </div>
                    <div class="setting-item admin-only">
                        <span class="setting-label">WiFi Settings</span>
                        <button class="btn btn-primary" onclick="showWiFiModal()">Configure</button>
                    </div>
                </div>
                
                <div class="settings-group">
                    <h3>Security</h3>
                    <div class="setting-item">
                        <span class="setting-label">Change Password</span>
                        <button class="btn btn-primary" onclick="showPasswordModal()">Change</button>
                    </div>
                </div>
                
                <div class="settings-group">
                    <h3>System</h3>
                    <div class="setting-item">
                        <span class="setting-label">Uptime</span>
                        <span class="setting-value" id="uptime">Loading...</span>
                    </div>
                    <div class="setting-item">
                        <span class="setting-label">Free Memory</span>
                        <span class="setting-value" id="memory">Loading...</span>
                    </div>
                </div>
            </div>
        </div>
    </div>
    
    <!-- WiFi Settings Modal -->
    <div id="wifiModal" class="modal">
        <div class="modal-content">
            <div class="modal-header">
                <h2>WiFi Settings</h2>
                <button class="close-btn" onclick="closeWiFiModal()">&times;</button>
            </div>
            <div id="wifiMessage" class="message"></div>
            <form id="wifiForm" onsubmit="saveWiFi(event)">
                <div class="form-group">
                    <label for="wifiSSID">WiFi SSID:</label>
                    <input type="text" id="wifiSSID" required>
                </div>
                <div class="form-group">
                    <label for="wifiPassword">WiFi Password:</label>
                    <input type="password" id="wifiPassword">
                </div>
                <div class="form-actions">
                    <button type="button" class="btn btn-secondary" onclick="closeWiFiModal()">Cancel</button>
                    <button type="submit" class="btn btn-primary">Save</button>
                </div>
            </form>
        </div>
    </div>
    
    <!-- Change Password Modal -->
    <div id="passwordModal" class="modal">
        <div class="modal-content">
            <div class="modal-header">
                <h2>Change Password</h2>
                <button class="close-btn" onclick="closePasswordModal()">&times;</button>
            </div>
            <div id="passwordMessage" class="message"></div>
            <form id="passwordForm" onsubmit="changePassword(event)">
                <div class="form-group">
                    <label for="currentPassword">Current Password:</label>
                    <input type="password" id="currentPassword" required>
                </div>
                <div class="form-group">
                    <label for="newPassword">New Password:</label>
                    <input type="password" id="newPassword" required minlength="4">
                </div>
                <div class="form-group">
                    <label for="confirmPassword">Confirm New Password:</label>
                    <input type="password" id="confirmPassword" required minlength="4">
                </div>
                <div class="form-actions">
                    <button type="button" class="btn btn-secondary" onclick="closePasswordModal()">Cancel</button>
                    <button type="submit" class="btn btn-primary">Change Password</button>
                </div>
            </form>
        </div>
    </div>
    
    <script>
        let userRole = '';
        
        // Initialize
        async function init() {
            await loadStatus();
            setInterval(loadStatus, 5000); // Update every 5 seconds
        }
        
        // Load system status
        async function loadStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                
                if (data.role) {
                    userRole = data.role;
                    document.getElementById('userInfo').textContent = 
                        `Logged in as: ${data.role === 'admin' ? 'Admin' : 'User'}`;
                    
                    // Show admin-only elements
                    if (userRole === 'admin') {
                        document.querySelectorAll('.admin-only').forEach(el => {
                            el.style.display = 'flex';
                        });
                    }
                }
                
                if (data.ip) {
                    document.getElementById('ipAddress').textContent = data.ip;
                }
                if (data.uptime) {
                    document.getElementById('uptime').textContent = formatUptime(data.uptime);
                }
                if (data.free_memory) {
                    document.getElementById('memory').textContent = 
                        (data.free_memory / 1024).toFixed(1) + ' KB';
                }
            } catch (error) {
                console.error('Failed to load status:', error);
            }
        }
        
        // Format uptime
        function formatUptime(seconds) {
            const days = Math.floor(seconds / 86400);
            const hours = Math.floor((seconds % 86400) / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            return `${days}d ${hours}h ${minutes}m`;
        }
        
        // Update camera settings
        async function updateCamera() {
            const settings = {
                resolution: document.getElementById('resolution').value,
                quality: document.getElementById('quality').value,
                brightness: document.getElementById('brightness').value,
                contrast: document.getElementById('contrast').value
            };
            
            try {
                const response = await fetch('/api/camera/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams(settings)
                });
                
                if (!response.ok) {
                    console.error('Failed to update camera settings');
                }
            } catch (error) {
                console.error('Error updating camera:', error);
            }
        }
        
        // WiFi Modal
        function showWiFiModal() {
            if (userRole !== 'admin') {
                alert('Admin access required');
                return;
            }
            document.getElementById('wifiModal').classList.add('show');
        }
        
        function closeWiFiModal() {
            document.getElementById('wifiModal').classList.remove('show');
            document.getElementById('wifiMessage').style.display = 'none';
            document.getElementById('wifiForm').reset();
        }
        
        async function saveWiFi(event) {
            event.preventDefault();
            const messageDiv = document.getElementById('wifiMessage');
            
            const formData = new URLSearchParams({
                wifiSSID: document.getElementById('wifiSSID').value,
                wifiPassword: document.getElementById('wifiPassword').value
            });
            
            try {
                const response = await fetch('/api/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: formData
                });
                
                const data = await response.json();
                
                if (response.ok) {
                    messageDiv.className = 'message success';
                    messageDiv.textContent = 'WiFi settings saved! Device will reconnect.';
                    messageDiv.style.display = 'block';
                    setTimeout(() => {
                        closeWiFiModal();
                        location.reload();
                    }, 2000);
                } else {
                    messageDiv.className = 'message error';
                    messageDiv.textContent = data.message || 'Failed to save WiFi settings';
                    messageDiv.style.display = 'block';
                }
            } catch (error) {
                messageDiv.className = 'message error';
                messageDiv.textContent = 'Error saving WiFi settings';
                messageDiv.style.display = 'block';
            }
        }
        
        // Password Modal
        function showPasswordModal() {
            document.getElementById('passwordModal').classList.add('show');
        }
        
        function closePasswordModal() {
            document.getElementById('passwordModal').classList.remove('show');
            document.getElementById('passwordMessage').style.display = 'none';
            document.getElementById('passwordForm').reset();
        }
        
        async function changePassword(event) {
            event.preventDefault();
            const messageDiv = document.getElementById('passwordMessage');
            
            const newPassword = document.getElementById('newPassword').value;
            const confirmPassword = document.getElementById('confirmPassword').value;
            
            if (newPassword !== confirmPassword) {
                messageDiv.className = 'message error';
                messageDiv.textContent = 'New passwords do not match';
                messageDiv.style.display = 'block';
                return;
            }
            
            const formData = new URLSearchParams({
                current_password: document.getElementById('currentPassword').value,
                new_password: newPassword,
                confirm_password: confirmPassword
            });
            
            try {
                const response = await fetch('/api/change-password', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: formData
                });
                
                const data = await response.json();
                
                if (response.ok) {
                    messageDiv.className = 'message success';
                    messageDiv.textContent = 'Password changed successfully!';
                    messageDiv.style.display = 'block';
                    setTimeout(() => {
                        closePasswordModal();
                    }, 2000);
                } else {
                    messageDiv.className = 'message error';
                    messageDiv.textContent = data.message || 'Failed to change password';
                    messageDiv.style.display = 'block';
                }
            } catch (error) {
                messageDiv.className = 'message error';
                messageDiv.textContent = 'Error changing password';
                messageDiv.style.display = 'block';
            }
        }
        
        // Logout
        async function logout() {
            try {
                await fetch('/api/logout', { method: 'POST' });
                window.location.href = '/';
            } catch (error) {
                console.error('Logout error:', error);
                window.location.href = '/';
            }
        }
        
        // Close modals when clicking outside
        window.onclick = function(event) {
            if (event.target.classList.contains('modal')) {
                event.target.classList.remove('show');
            }
        }
        
        // Initialize on load
        init();
    </script>
</body>
</html>
)HTML";
    
    request->send(200, "text/html", html);
}
```

**Key Improvements:**
1. ✅ **Modal dialogs** for WiFi settings and password change (hidden by default)
2. ✅ **Confirm password** field added with validation
3. ✅ **Larger logout button** (16px font, 12px/24px padding)
4. ✅ **Role-based visibility** (admin-only sections hidden for users)
5. ✅ **Better UX** with animations and clear visual feedback
6. ✅ **Responsive design** for mobile devices

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

### **Phase 7: Python Client Examples (Optional - Enhanced with GUI)**
**Architecture:** Python is now a simple MJPEG client. No ESP32 code changes needed!

**Reference:** Use `/Users/szemy/Workspace/Genican/ESP32P4_video_transmission/tools/rtsp_capture_viewer.py` as template

**Create `python_clients/` directory with enhanced examples:**

#### **camera_viewer.py** - Full-featured PyQt6 GUI viewer
```python
#!/usr/bin/env python3
"""ESP32 Camera Viewer with auto-capture and settings persistence"""

import json
import os
from pathlib import Path
from datetime import datetime
from queue import Queue, Empty
import threading
import time

import cv2
from PyQt6.QtCore import QTimer, Qt
from PyQt6.QtGui import QImage, QPixmap
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QLineEdit, QDialog, QDialogButtonBox,
    QMessageBox, QFileDialog
)

DEFAULT_SETTINGS = {
    "stream_url": "http://192.168.2.100/stream",
    "capture_dir": "./captures",
    "auto_capture_enabled": True,
    "capture_interval": 1.0,
    "window_width": 900,
    "window_height": 760
}

class Settings:
    def __init__(self, config_file='settings.json'):
        self.config_file = Path(config_file)
        self.data = self.load()
    
    def load(self):
        if self.config_file.exists():
            with open(self.config_file, 'r') as f:
                return {**DEFAULT_SETTINGS, **json.load(f)}
        return DEFAULT_SETTINGS.copy()
    
    def save(self):
        with open(self.config_file, 'w') as f:
            json.dump(self.data, f, indent=2)
    
    def get(self, key, default=None):
        return self.data.get(key, default)
    
    def set(self, key, value):
        self.data[key] = value
        self.save()

class VideoLabel(QLabel):
    def __init__(self):
        super().__init__("Connecting...")
        self.source_image = None
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet("background-color: #000; color: white;")
        self.setMinimumSize(640, 480)
    
    def set_image(self, image):
        self.source_image = image
        self.refresh_pixmap()
    
    def refresh_pixmap(self):
        if self.source_image:
            pixmap = QPixmap.fromImage(self.source_image).scaled(
                self.size(),
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation
            )
            self.setPixmap(pixmap)

class CameraViewer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.settings = Settings()
        self.capture = None
        self.stop_event = threading.Event()
        self.capture_enabled = self.settings.get('auto_capture_enabled')
        self.next_capture_time = 0.0
        self.frame_queue = Queue(maxsize=1)
        self.save_queue = Queue(maxsize=2)
        
        self.setup_ui()
        self.start_workers()
    
    def setup_ui(self):
        self.setWindowTitle("ESP32 Camera Viewer")
        self.resize(
            self.settings.get('window_width'),
            self.settings.get('window_height')
        )
        
        central = QWidget()
        layout = QVBoxLayout(central)
        
        # Video display
        self.video = VideoLabel()
        layout.addWidget(self.video, 1)
        
        # Controls
        controls = QWidget()
        controls_layout = QHBoxLayout(controls)
        
        # Status
        self.status = QLabel("Connecting...")
        controls_layout.addWidget(self.status, 1)
        
        # Capture button
        self.capture_button = QPushButton(
            "Stop Capture" if self.capture_enabled else "Start Capture"
        )
        self.capture_button.clicked.connect(self.toggle_capture)
        controls_layout.addWidget(self.capture_button)
        
        # Settings button
        settings_btn = QPushButton("Settings")
        settings_btn.clicked.connect(self.show_settings)
        controls_layout.addWidget(settings_btn)
        
        # Quit button
        quit_btn = QPushButton("Quit")
        quit_btn.clicked.connect(self.close)
        controls_layout.addWidget(quit_btn)
        
        layout.addWidget(controls)
        self.setCentralWidget(central)
        
        # Poll timer
        self.poll_timer = QTimer(self)
        self.poll_timer.setInterval(30)
        self.poll_timer.timeout.connect(self.poll_workers)
        self.poll_timer.start()
    
    def start_workers(self):
        self.stream_thread = threading.Thread(
            target=self.stream_worker, daemon=True
        )
        self.save_thread = threading.Thread(
            target=self.save_worker, daemon=True
        )
        self.stream_thread.start()
        self.save_thread.start()
    
    def stream_worker(self):
        url = self.settings.get('stream_url')
        self.capture = cv2.VideoCapture(url)
        
        if not self.capture.isOpened():
            QMessageBox.critical(self, "Error", f"Cannot connect to {url}")
            return
        
        frame_times = []
        while not self.stop_event.is_set():
            ret, frame = self.capture.read()
            if not ret:
                break
            
            now = time.time()
            frame_times.append(now)
            if len(frame_times) > 30:
                frame_times.pop(0)
            
            fps = len(frame_times) / (frame_times[-1] - frame_times[0]) if len(frame_times) > 1 else 0
            
            # Auto-capture
            if self.capture_enabled and now >= self.next_capture_time:
                interval = self.settings.get('capture_interval')
                self.next_capture_time = now + interval
                try:
                    self.save_queue.put_nowait(frame.copy())
                except:
                    pass
            
            # Display
            try:
                self.frame_queue.put_nowait((frame, fps))
            except:
                pass
    
    def save_worker(self):
        capture_dir = Path(self.settings.get('capture_dir'))
        capture_dir.mkdir(parents=True, exist_ok=True)
        
        while not self.stop_event.is_set():
            try:
                frame = self.save_queue.get(timeout=0.1)
            except Empty:
                continue
            
            if self.capture_enabled:
                timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
                filename = capture_dir / f'capture_{timestamp}.jpg'
                cv2.imwrite(str(filename), frame)
    
    def poll_workers(self):
        try:
            frame, fps = self.frame_queue.get_nowait()
            h, w = frame.shape[:2]
            
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            image = QImage(rgb.data, w, h, rgb.strides[0], 
                          QImage.Format.Format_RGB888).copy()
            self.video.set_image(image)
            self.status.setText(f"{w}x{h} | {fps:.1f} FPS")
        except Empty:
            pass
    
    def toggle_capture(self):
        self.capture_enabled = not self.capture_enabled
        self.capture_button.setText(
            "Stop Capture" if self.capture_enabled else "Start Capture"
        )
        self.settings.set('auto_capture_enabled', self.capture_enabled)
    
    def show_settings(self):
        # TODO: Implement settings dialog
        pass
    
    def closeEvent(self, event):
        self.settings.set('window_width', self.width())
        self.settings.set('window_height', self.height())
        self.stop_event.set()
        if self.capture:
            self.capture.release()
        event.accept()

if __name__ == "__main__":
    import sys
    app = QApplication(sys.argv)
    viewer = CameraViewer()
    viewer.show()
    sys.exit(app.exec())
```

#### **settings.json** - Configuration file
```json
{
  "stream_url": "http://192.168.2.100/stream",
  "capture_dir": "./captures",
  "auto_capture_enabled": true,
  "capture_interval": 1.0,
  "window_width": 900,
  "window_height": 760,
  "connection_timeout": 10000,
  "read_timeout": 3000
}
```

#### **requirements.txt**
```
opencv-python>=4.8.0
PyQt6>=6.5.0
numpy>=1.24.0
requests>=2.31.0
```

#### **stream_viewer_simple.py** - Simple OpenCV viewer (no settings)
```python
import cv2
import argparse

def view_stream(stream_url):
    """Display live MJPEG stream from ESP32"""
    cap = cv2.VideoCapture(stream_url)
    
    if not cap.isOpened():
        print(f"Failed to connect to {stream_url}")
        return
    
    print("Connected! Press 'q' to quit")
    
    while True:
        ret, frame = cap.read()
        if not ret:
            print("Stream ended")
            break
        
        cv2.imshow('ESP32 Camera', frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--url', default='http://192.168.2.100/stream')
    args = parser.parse_args()
    view_stream(args.url)
```

#### **README.md for python_clients**
```markdown
# ESP32 Camera Python Clients

Professional Python clients for ESP32 camera with MJPEG streaming.

## Features

- **camera_viewer.py**: Full-featured PyQt6 GUI with:
  - Live stream display
  - Auto-capture (1 frame/second)
  - Settings persistence (JSON)
  - FPS counter
  - Start/Stop capture toggle
  
- **stream_viewer_simple.py**: Lightweight OpenCV viewer
- **video_recorder.py**: Record video files
- **motion_detector.py**: Motion detection
- **cloud_uploader.py**: Upload to cloud

## Installation

```bash
pip install -r requirements.txt
```

## Usage

### Full GUI Viewer
```bash
python camera_viewer.py
```

Settings are saved to `settings.json` automatically.

### Simple Viewer
```bash
python stream_viewer_simple.py --url http://192.168.2.100/stream
```

### Record Video
```bash
python video_recorder.py --url http://192.168.2.100/stream --output recordings
```

## Configuration

Edit `settings.json`:

```json
{
  "stream_url": "http://192.168.2.100/stream",
  "capture_dir": "./captures",
  "auto_capture_enabled": true,
  "capture_interval": 1.0
}
```

## Controlling Camera via API

Python can also control the ESP32 camera:

```python
import requests

# Change resolution
requests.post('http://192.168.2.100/api/camera/config', 
              json={'resolution': 'UXGA', 'quality': 10})

# Get status
response = requests.get('http://192.168.2.100/api/status')
print(response.json())

# Capture single frame
response = requests.get('http://192.168.2.100/capture')
with open('snapshot.jpg', 'wb') as f:
    f.write(response.content)
```

## Reference

Based on ESP32-P4 RTSP Viewer architecture.
```

**Note:** The complete `camera_viewer.py` implementation should follow the RTSP Viewer pattern at:
`/Users/szemy/Workspace/Genican/ESP32P4_video_transmission/tools/rtsp_capture_viewer.py`

Key features to implement:
- Threaded frame capture and saving
- Queue-based frame passing
- Connection dialog with countdown
- Error handling dialogs
- Status notifications
- Settings persistence

**No ESP32 code changes needed for Phase 7!**

---

### **Phase 7B: Professional Camera Viewer Application**

**Goal:** Create a unified, beautiful PyQt6 application with advanced capture features including burst mode with charging animation and video recording.

#### **Project Structure**

```
python_clients/professional_viewer/
├── camera_viewer_pro.py          # Main application entry
├── config.json                   # Default configuration
├── requirements.txt              # Dependencies
├── README.md                     # Documentation
├── icon.png                      # Application icon
├── widgets/
│   ├── __init__.py
│   ├── stream_widget.py          # Live preview with OpenCV
│   ├── burst_button.py           # Custom button with charging animation
│   ├── capture_controls.py       # Control panel widget
│   └── settings_dialog.py        # Settings dialog
├── workers/
│   ├── __init__.py
│   ├── stream_worker.py          # Stream reading thread (QThread)
│   ├── save_worker.py            # File saving thread
│   └── video_worker.py           # Video encoding thread
├── utils/
│   ├── __init__.py
│   ├── config_manager.py         # Configuration file handler
│   └── file_manager.py           # Smart filename generation
└── styles/
    ├── __init__.py
    └── dark_theme.qss            # Dark theme stylesheet
```

#### **7B.1: Main Application (camera_viewer_pro.py)**

```python
#!/usr/bin/env python3
"""
ESP32 Camera Professional Viewer
Advanced camera viewer with burst mode and video recording
"""

import sys
import json
from pathlib import Path
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QPushButton, QLabel, QStatusBar,
                             QLineEdit, QFileDialog)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QSize
from PyQt6.QtGui import QIcon, QFont

from widgets.stream_widget import StreamWidget
from widgets.burst_button import BurstButton
from widgets.settings_dialog import SettingsDialog
from workers.stream_worker import StreamWorker
from workers.save_worker import SaveWorker
from workers.video_worker import VideoWorker
from utils.config_manager import ConfigManager
from utils.file_manager import FileManager


class CameraViewerPro(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("ESP32 Camera Viewer Pro")
        self.setMinimumSize(1000, 800)
        
        # Configuration
        self.config = ConfigManager()
        self.file_manager = FileManager(self.config.get('capture.save_directory'))
        
        # Workers
        self.stream_worker = None
        self.save_worker = SaveWorker()
        self.video_worker = VideoWorker()
        
        # State
        self.is_recording = False
        self.burst_active = False
        self.frame_count = 0
        
        # Setup UI
        self.init_ui()
        self.load_stylesheet()
        self.restore_geometry()
        
        # Start stream
        self.connect_stream()
        
    def init_ui(self):
        """Initialize user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # Top bar - Connection
        top_bar = QHBoxLayout()
        top_bar.addWidget(QLabel("Stream URL:"))
        self.url_input = QLineEdit(self.config.get('stream.url'))
        self.url_input.returnPressed.connect(self.reconnect_stream)
        top_bar.addWidget(self.url_input)
        self.connect_btn = QPushButton("Reconnect")
        self.connect_btn.clicked.connect(self.reconnect_stream)
        top_bar.addWidget(self.connect_btn)
        main_layout.addLayout(top_bar)
        
        # Stream preview
        self.stream_widget = StreamWidget()
        self.stream_widget.setMinimumSize(800, 600)
        main_layout.addWidget(self.stream_widget, stretch=1)
        
        # Control panel
        controls = self.create_control_panel()
        main_layout.addLayout(controls)
        
        # Status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        
        self.connection_label = QLabel("● Disconnected")
        self.connection_label.setStyleSheet("color: #ff4444;")
        self.status_bar.addPermanentWidget(self.connection_label)
        
        self.stats_label = QLabel("Captured: 0 photos | 0 videos")
        self.status_bar.addPermanentWidget(self.stats_label)
        
    def create_control_panel(self):
        """Create capture control panel"""
        controls = QHBoxLayout()
        
        # Single capture button
        self.capture_btn = QPushButton("📷 Capture")
        self.capture_btn.setMinimumSize(120, 60)
        self.capture_btn.clicked.connect(self.capture_single)
        controls.addWidget(self.capture_btn)
        
        # Burst mode button (with long press)
        self.burst_btn = BurstButton("⏺ Burst")
        self.burst_btn.setMinimumSize(120, 60)
        self.burst_btn.pressed_signal.connect(self.start_burst)
        self.burst_btn.released_signal.connect(self.stop_burst)
        self.burst_btn.charging_signal.connect(self.update_burst_charging)
        controls.addWidget(self.burst_btn)
        
        # Video recording button
        self.record_btn = QPushButton("🎥 Record")
        self.record_btn.setMinimumSize(120, 60)
        self.record_btn.setCheckable(True)
        self.record_btn.clicked.connect(self.toggle_recording)
        controls.addWidget(self.record_btn)
        
        # Settings button
        self.settings_btn = QPushButton("⚙ Settings")
        self.settings_btn.setMinimumSize(120, 60)
        self.settings_btn.clicked.connect(self.show_settings)
        controls.addWidget(self.settings_btn)
        
        controls.addStretch()
        return controls
        
    def connect_stream(self):
        """Connect to MJPEG stream"""
        url = self.url_input.text()
        self.stream_worker = StreamWorker(url)
        self.stream_worker.frame_ready.connect(self.stream_widget.update_frame)
        self.stream_worker.fps_update.connect(self.update_fps)
        self.stream_worker.error_signal.connect(self.handle_stream_error)
        self.stream_worker.connected_signal.connect(self.on_stream_connected)
        self.stream_worker.start()
        
    def reconnect_stream(self):
        """Reconnect to stream with new URL"""
        if self.stream_worker:
            self.stream_worker.stop()
            self.stream_worker.wait()
        self.connect_stream()
        
    def on_stream_connected(self):
        """Handle stream connection success"""
        self.connection_label.setText("● Connected")
        self.connection_label.setStyleSheet("color: #44ff44;")
        self.status_bar.showMessage("Stream connected", 3000)
        
    def handle_stream_error(self, error):
        """Handle stream errors"""
        self.connection_label.setText("● Error")
        self.connection_label.setStyleSheet("color: #ff4444;")
        self.status_bar.showMessage(f"Stream error: {error}", 5000)
        
    def update_fps(self, fps):
        """Update FPS display"""
        self.stream_widget.set_fps(fps)
        
    def capture_single(self):
        """Capture single frame"""
        frame = self.stream_widget.get_current_frame()
        if frame is not None:
            filename = self.file_manager.generate_photo_filename()
            self.save_worker.save_frame(frame, filename)
            self.status_bar.showMessage(f"Saved: {filename}", 3000)
            self.update_stats()
            
    def start_burst(self):
        """Start burst capture mode"""
        self.burst_active = True
        self.frame_count = 0
        fps = self.config.get('capture.burst_fps', 5)
        interval = int(1000 / fps)  # milliseconds
        
        self.burst_timer = QTimer()
        self.burst_timer.timeout.connect(self.capture_burst_frame)
        self.burst_timer.start(interval)
        
        self.status_bar.showMessage("Burst mode active...", 0)
        
    def capture_burst_frame(self):
        """Capture one frame during burst"""
        frame = self.stream_widget.get_current_frame()
        if frame is not None:
            self.frame_count += 1
            filename = self.file_manager.generate_burst_filename(self.frame_count)
            self.save_worker.save_frame(frame, filename)
            self.burst_btn.set_frame_count(self.frame_count)
            
    def stop_burst(self):
        """Stop burst capture mode"""
        if hasattr(self, 'burst_timer'):
            self.burst_timer.stop()
        self.burst_active = False
        self.status_bar.showMessage(f"Burst complete: {self.frame_count} frames", 3000)
        self.burst_btn.reset()
        self.update_stats()
        
    def update_burst_charging(self, progress):
        """Update burst button charging animation"""
        # Progress is 0.0 to 1.0
        pass  # Animation handled by BurstButton widget
        
    def toggle_recording(self, checked):
        """Toggle video recording"""
        if checked:
            # Prompt for filename
            default_name = self.file_manager.generate_video_filename()
            filename, _ = QFileDialog.getSaveFileName(
                self, "Save Video", default_name, 
                "Video Files (*.avi *.mp4);;All Files (*)"
            )
            
            if filename:
                self.start_recording(filename)
            else:
                self.record_btn.setChecked(False)
        else:
            self.stop_recording()
            
    def start_recording(self, filename):
        """Start video recording"""
        fps = self.config.get('video.fps', 20)
        codec = self.config.get('video.codec', 'MJPEG')
        
        self.video_worker.start_recording(filename, fps, codec)
        self.stream_widget.frame_updated.connect(self.video_worker.add_frame)
        
        self.is_recording = True
        self.record_btn.setText("⏹ Stop")
        self.record_btn.setStyleSheet("background-color: #ff4444;")
        self.status_bar.showMessage(f"Recording to: {filename}", 0)
        
    def stop_recording(self):
        """Stop video recording"""
        self.stream_widget.frame_updated.disconnect(self.video_worker.add_frame)
        self.video_worker.stop_recording()
        
        self.is_recording = False
        self.record_btn.setText("🎥 Record")
        self.record_btn.setStyleSheet("")
        self.status_bar.showMessage("Recording stopped", 3000)
        self.update_stats()
        
    def show_settings(self):
        """Show settings dialog"""
        dialog = SettingsDialog(self.config, self)
        if dialog.exec():
            # Settings saved, reload if needed
            self.file_manager.save_directory = self.config.get('capture.save_directory')
            
    def update_stats(self):
        """Update statistics display"""
        photo_count = len(list(Path(self.file_manager.save_directory).glob("capture_*.jpg")))
        video_count = len(list(Path(self.file_manager.save_directory).glob("*.avi")))
        self.stats_label.setText(f"Captured: {photo_count} photos | {video_count} videos")
        
    def load_stylesheet(self):
        """Load dark theme stylesheet"""
        theme = self.config.get('ui.theme', 'dark')
        if theme == 'dark':
            style_file = Path(__file__).parent / 'styles' / 'dark_theme.qss'
            if style_file.exists():
                with open(style_file, 'r') as f:
                    self.setStyleSheet(f.read())
                    
    def restore_geometry(self):
        """Restore window geometry from config"""
        geometry = self.config.get('ui.window_geometry')
        if geometry:
            self.setGeometry(*geometry)
            
    def closeEvent(self, event):
        """Handle application close"""
        # Save window geometry
        geo = self.geometry()
        self.config.set('ui.window_geometry', [geo.x(), geo.y(), geo.width(), geo.height()])
        self.config.save()
        
        # Stop workers
        if self.stream_worker:
            self.stream_worker.stop()
            self.stream_worker.wait()
        if self.is_recording:
            self.stop_recording()
            
        event.accept()


def main():
    app = QApplication(sys.argv)
    app.setApplicationName("ESP32 Camera Viewer Pro")
    
    # Set font
    font = QFont("Segoe UI", 10)
    app.setFont(font)
    
    window = CameraViewerPro()
    window.show()
    
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
```

#### **7B.2: Burst Button with Charging Animation (widgets/burst_button.py)**

```python
from PyQt6.QtWidgets import QPushButton
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QPointF
from PyQt6.QtGui import QPainter, QColor, QPen, QBrush, QFont


class BurstButton(QPushButton):
    """Custom button with long-press detection and charging animation"""
    
    pressed_signal = pyqtSignal()
    released_signal = pyqtSignal()
    charging_signal = pyqtSignal(float)  # Progress 0.0-1.0
    
    def __init__(self, text="Burst", parent=None):
        super().__init__(text, parent)
        
        self.press_duration = 0
        self.charge_threshold = 200  # 200ms before burst starts
        self.frame_count = 0
        self.is_charging = False
        
        # Timer for press duration tracking
        self.press_timer = QTimer()
        self.press_timer.timeout.connect(self.update_charging)
        self.press_timer.setInterval(50)  # Update every 50ms
        
    def mousePressEvent(self, event):
        """Handle mouse press"""
        super().mousePressEvent(event)
        if event.button() == Qt.MouseButton.LeftButton:
            self.press_duration = 0
            self.is_charging = True
            self.press_timer.start()
            
    def mouseReleaseEvent(self, event):
        """Handle mouse release"""
        super().mouseReleaseEvent(event)
        if event.button() == Qt.MouseButton.LeftButton:
            self.press_timer.stop()
            
            if self.press_duration >= self.charge_threshold:
                self.released_signal.emit()
                
            self.is_charging = False
            self.press_duration = 0
            self.update()
            
    def update_charging(self):
        """Update charging animation"""
        self.press_duration += 50
        
        # Emit burst start after threshold
        if self.press_duration == self.charge_threshold:
            self.pressed_signal.emit()
            
        # Calculate progress (0.0 to 1.0)
        progress = min(1.0, (self.press_duration - self.charge_threshold) / 2000.0)
        self.charging_signal.emit(progress)
        
        self.update()  # Trigger repaint
        
    def paintEvent(self, event):
        """Custom paint with charging ring"""
        super().paintEvent(event)
        
        if not self.is_charging and self.frame_count == 0:
            return
            
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Calculate ring position and size
        rect = self.rect()
        center = rect.center()
        radius = min(rect.width(), rect.height()) // 2 - 10
        
        # Draw charging ring
        if self.press_duration >= self.charge_threshold:
            progress = (self.press_duration - self.charge_threshold) / 2000.0
            progress = min(1.0, progress)
            
            # Color transition: blue -> green -> yellow
            if progress < 0.5:
                color = QColor(66, 135, 245)  # Blue
            elif progress < 0.8:
                color = QColor(76, 209, 55)  # Green
            else:
                color = QColor(245, 166, 35)  # Yellow
                
            # Draw arc
            pen = QPen(color, 4)
            painter.setPen(pen)
            painter.setBrush(Qt.BrushStyle.NoBrush)
            
            start_angle = 90 * 16  # Start from top
            span_angle = int(-progress * 360 * 16)  # Clockwise
            
            painter.drawArc(center.x() - radius, center.y() - radius,
                          radius * 2, radius * 2, start_angle, span_angle)
                          
        # Draw frame count
        if self.frame_count > 0:
            font = QFont("Arial", 12, QFont.Weight.Bold)
            painter.setFont(font)
            painter.setPen(QColor(255, 255, 255))
            painter.drawText(rect, Qt.AlignmentFlag.AlignCenter, str(self.frame_count))
            
    def set_frame_count(self, count):
        """Set frame count display"""
        self.frame_count = count
        self.update()
        
    def reset(self):
        """Reset button state"""
        self.frame_count = 0
        self.press_duration = 0
        self.is_charging = False
        self.update()
```

**Note:** This is a comprehensive Phase 7B outline. The full implementation guide with all widgets, workers, and utilities would be extensive. Should I continue with the remaining components (StreamWidget, Workers, ConfigManager, etc.)?

### **Phase 8: Additional Features + WiFi Provisioning**

This phase implements advanced features for production use, including first-time setup mode and dual-tier authentication.

#### **8.0 Dual-Tier Authentication System**

This system provides two levels of access:
- **Admin**: Full access to all settings (WiFi, network, passwords, camera settings, stream)
- **User**: Limited access to camera functions only (stream viewing, camera settings)

**Implementation Notes:**
- Already implemented in Phase 1 (Storage) and Phase 2 (Web Server)
- Two separate credential sets stored in NVS
- Two separate session tokens managed independently
- Each API endpoint checks appropriate access level

**Access Control Matrix:**

| Endpoint | Admin | User | Public |
|----------|-------|------|--------|
| `/stream` | ✅ | ✅ | ❌ |
| `/capture` | ✅ | ✅ | ❌ |
| `/api/camera/config` | ✅ | ✅ | ❌ |
| `/api/settings` (GET) | ✅ | ❌ | ❌ |
| `/api/settings` (POST) | ✅ | ❌ | ❌ |
| `/api/change-password` | ✅ | ✅* | ❌ |
| `/api/restart` | ✅ | ❌ | ❌ |
| `/api/factory-reset` | ✅ | ❌ | ❌ |

*Users can only change their own password, not admin password.

**Update Required Endpoints:**

In `web_server.cpp`, update endpoints to check admin access:

```cpp
// Settings endpoints - ADMIN ONLY
void CameraWebServer::handleGetSettings(AsyncWebServerRequest *request) {
    if (!isAdminAuthenticated(request)) {
        sendJsonResponse(request, 403, "error", "Admin access required");
        return;
    }
    // ... existing implementation ...
}

void CameraWebServer::handlePostSettings(AsyncWebServerRequest *request) {
    if (!isAdminAuthenticated(request)) {
        sendJsonResponse(request, 403, "error", "Admin access required");
        return;
    }
    // ... existing implementation ...
}

// Change password - USER can change own password, ADMIN can change any
void CameraWebServer::handleChangePassword(AsyncWebServerRequest *request) {
    AuthLevel authLevel = getAuthLevel(request);
    if (authLevel == AuthLevel::NONE) {
        send401Unauthorized(request);
        return;
    }
    
    if (!request->hasArg("newPassword")) {
        sendJsonResponse(request, 400, "error", "Missing new password");
        return;
    }
    
    String newPassword = request->arg("newPassword");
    String targetUser = request->hasArg("targetUser") ? request->arg("targetUser") : "";
    
    // Users can only change their own password
    if (authLevel == AuthLevel::USER) {
        if (targetUser != "" && targetUser != "user") {
            sendJsonResponse(request, 403, "error", "Users can only change their own password");
            return;
        }
        settings_->writeUserPassword(newPassword.c_str(), newPassword.length());
        sendJsonResponse(request, 200, "success", "Password changed successfully");
        return;
    }
    
    // Admins can change any password
    if (authLevel == AuthLevel::ADMIN) {
        if (targetUser == "user") {
            settings_->writeUserPassword(newPassword.c_str(), newPassword.length());
        } else {
            settings_->writeAdminPassword(newPassword.c_str(), newPassword.length());
        }
        sendJsonResponse(request, 200, "success", "Password changed successfully");
        return;
    }
}

// Camera endpoints - BOTH ADMIN and USER
void CameraWebServer::handleCameraConfig(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {  // Any authenticated user
        send401Unauthorized(request);
        return;
    }
    // ... existing implementation ...
}

void CameraWebServer::handleStream(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {  // Any authenticated user
        send401Unauthorized(request);
        return;
    }
    // ... existing implementation ...
}

void CameraWebServer::handleCapture(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {  // Any authenticated user
        send401Unauthorized(request);
        return;
    }
    // ... existing implementation ...
}
```

**Update WiFi Provisioning Setup Page:**

Modify `wifi_provisioning.cpp` to set up both admin and user credentials during first-time setup:

```cpp
// In setupWebHandlers() method, update /api/configure endpoint:
setupServer->on("/api/configure", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
        return;
    }
    
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    String adminUser = request->hasParam("adminUsername", true) ? 
                      request->getParam("adminUsername", true)->value() : "admin";
    String adminPass = request->hasParam("adminPassword", true) ? 
                      request->getParam("adminPassword", true)->value() : "admin";
    String userUser = request->hasParam("userUsername", true) ? 
                     request->getParam("userUsername", true)->value() : "user";
    String userPass = request->hasParam("userPassword", true) ? 
                     request->getParam("userPassword", true)->value() : "user";
    
    // Save all credentials
    settings->writeWifiSSID(ssid.c_str(), ssid.length());
    settings->writeWifiPassword(password.c_str(), password.length());
    settings->writeAdminUsername(adminUser.c_str(), adminUser.length());
    settings->writeAdminPassword(adminPass.c_str(), adminPass.length());
    settings->writeUserUsername(userUser.c_str(), userUser.length());
    settings->writeUserPassword(userPass.c_str(), userPass.length());
    settings->setWiFiConfigured(true);
    
    // ... rest of connection logic ...
});
```

**Update Setup Page HTML:**

In `getSetupPageHTML()` method, add user credential fields:

```html
<div class="form-group">
    <label for="adminUsername">Admin Username</label>
    <input type="text" id="adminUsername" value="admin" required>
</div>

<div class="form-group">
    <label for="adminPassword">Admin Password</label>
    <input type="password" id="adminPassword" value="admin" required>
</div>

<div class="form-group">
    <label for="userUsername">User Username (Camera Access)</label>
    <input type="text" id="userUsername" value="user" required>
</div>

<div class="form-group">
    <label for="userPassword">User Password</label>
    <input type="password" id="userPassword" value="user" required>
</div>
```

And update the JavaScript to send all fields:

```javascript
formData.append('adminUsername', document.getElementById('adminUsername').value);
formData.append('adminPassword', document.getElementById('adminPassword').value);
formData.append('userUsername', document.getElementById('userUsername').value);
formData.append('userPassword', document.getElementById('userPassword').value);
```

#### **8.1 Storage: Add WiFi Provisioning Flag**

First, update `camera_settings.h` to add the provisioning flag:

```cpp
// Add to CameraSettings class
class CameraSettings {
public:
    // ... existing members ...
    bool isWiFiConfigured;  // Flag for first boot detection
    
    struct DefaultValues {
        // ... existing defaults ...
        static constexpr bool IS_WIFI_CONFIGURED = false;
    };
    
    // Add methods
    bool checkWiFiConfigured();
    void setWiFiConfigured(bool configured);
};
```

In `camera_settings.cpp`, add read/write methods:

```cpp
bool CameraSettings::checkWiFiConfigured() {
    prefs.begin(NVS_NAMESPACE, true); // read-only
    bool configured = prefs.getBool("wifiConfigured", false);
    prefs.end();
    return configured;
}

void CameraSettings::setWiFiConfigured(bool configured) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("wifiConfigured", configured);
    prefs.end();
    isWiFiConfigured = configured;
}
```

#### **8.2 WiFi Provisioning Mode (AP + Captive Portal)**

Create `wifi_provisioning.h`:

```cpp
#ifndef __WIFI_PROVISIONING_H__
#define __WIFI_PROVISIONING_H__

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include "camera_settings.h"

class WiFiProvisioning {
private:
    static constexpr char AP_SSID[] = "ESP32-CAM-Setup";
    static constexpr char AP_PASSWORD[] = "12345678";  // or "" for open network
    static constexpr uint8_t DNS_PORT = 53;
    static constexpr IPAddress AP_IP;
    static constexpr IPAddress AP_GATEWAY;
    static constexpr IPAddress AP_SUBNET;
    
    DNSServer* dnsServer;
    AsyncWebServer* setupServer;
    CameraSettings* settings;
    bool connectionInProgress;
    
public:
    WiFiProvisioning(CameraSettings* settings);
    ~WiFiProvisioning();
    
    bool startAPMode();
    void stopAPMode();
    void handleDNS();  // Call in loop
    bool isActive();
    
private:
    void setupWebHandlers();
    String getSetupPageHTML();
    String getAvailableNetworksJSON();
};

#endif
```

Create `wifi_provisioning.cpp`:

```cpp
#include "wifi_provisioning.h"

constexpr IPAddress WiFiProvisioning::AP_IP(192, 168, 4, 1);
constexpr IPAddress WiFiProvisioning::AP_GATEWAY(192, 168, 4, 1);
constexpr IPAddress WiFiProvisioning::AP_SUBNET(255, 255, 255, 0);

WiFiProvisioning::WiFiProvisioning(CameraSettings* settings) 
    : settings(settings), connectionInProgress(false) {
    dnsServer = new DNSServer();
    setupServer = new AsyncWebServer(80);
}

WiFiProvisioning::~WiFiProvisioning() {
    stopAPMode();
    delete dnsServer;
    delete setupServer;
}

bool WiFiProvisioning::startAPMode() {
    Serial.println("Starting AP mode for WiFi provisioning...");
    
    // Configure AP
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (!apStarted) {
        Serial.println("Failed to start AP mode!");
        return false;
    }
    
    Serial.print("AP started. IP: ");
    Serial.println(WiFi.softAPIP());
    
    // Start DNS server for captive portal
    dnsServer->start(DNS_PORT, "*", AP_IP);
    
    // Start mDNS
    MDNS.begin("camera");
    
    // Setup web handlers
    setupWebHandlers();
    setupServer->begin();
    
    return true;
}

void WiFiProvisioning::stopAPMode() {
    dnsServer->stop();
    setupServer->end();
    WiFi.softAPdisconnect(true);
    MDNS.end();
}

void WiFiProvisioning::handleDNS() {
    dnsServer->processNextRequest();
}

bool WiFiProvisioning::isActive() {
    return WiFi.getMode() == WIFI_AP;
}

void WiFiProvisioning::setupWebHandlers() {
    // Captive portal: redirect all requests to setup page
    setupServer->onNotFound([this](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    // Setup page
    setupServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getSetupPageHTML());
    });
    
    // Scan WiFi networks
    setupServer->on("/api/scan", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", getAvailableNetworksJSON());
    });
    
    // Submit WiFi credentials
    setupServer->on("/api/configure", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
            return;
        }
        
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        String username = request->hasParam("username", true) ? 
                         request->getParam("username", true)->value() : "admin";
        String adminPass = request->hasParam("adminPassword", true) ? 
                          request->getParam("adminPassword", true)->value() : "admin";
        
        // Save credentials
        settings->writeWifiSSID(ssid.c_str(), ssid.length());
        settings->writeWifiPassword(password.c_str(), password.length());
        settings->writeUsername(username.c_str(), username.length());
        settings->writePassword(adminPass.c_str(), adminPass.length());
        settings->setWiFiConfigured(true);
        
        // Try to connect
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        
        // Wait 10 seconds for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            String ip = WiFi.localIP().toString();
            request->send(200, "application/json", 
                "{\"success\":true,\"message\":\"Connected! IP: " + ip + "\"}");
            
            // Reboot after 3 seconds
            delay(3000);
            ESP.restart();
        } else {
            settings->setWiFiConfigured(false);  // Reset flag on failure
            WiFi.mode(WIFI_AP);  // Back to AP mode
            request->send(200, "application/json", 
                "{\"success\":false,\"error\":\"Failed to connect. Check password.\"}");
        }
    });
}

String WiFiProvisioning::getAvailableNetworksJSON() {
    int n = WiFi.scanNetworks();
    String json = "[";
    
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i));
        json += "}";
    }
    
    json += "]";
    return json;
}

String WiFiProvisioning::getSetupPageHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Camera Setup</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
        }
        .setup-container {
            background: white;
            border-radius: 12px;
            padding: 40px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            max-width: 400px;
            width: 100%;
        }
        h1 {
            color: #333;
            margin-top: 0;
            font-size: 24px;
            text-align: center;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #555;
            font-weight: 500;
        }
        input, select {
            width: 100%;
            padding: 12px;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 14px;
            box-sizing: border-box;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        .btn {
            width: 100%;
            padding: 12px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            margin-top: 10px;
        }
        .btn:hover {
            background: #5568d3;
        }
        .btn:disabled {
            background: #ccc;
            cursor: not-allowed;
        }
        .signal-bars {
            display: inline-block;
            margin-left: 10px;
        }
        .message {
            padding: 12px;
            border-radius: 6px;
            margin-bottom: 15px;
            display: none;
        }
        .message.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .message.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 20px;
            height: 20px;
            animation: spin 1s linear infinite;
            display: inline-block;
            margin-right: 10px;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="setup-container">
        <h1>📷 ESP32 Camera Setup</h1>
        
        <div id="message" class="message"></div>
        
        <form id="setupForm">
            <div class="form-group">
                <label for="ssid">WiFi Network</label>
                <select id="ssid" required>
                    <option value="">Scanning networks...</option>
                </select>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password</label>
                <input type="password" id="password" required>
            </div>
            
            <div class="form-group">
                <label for="username">Admin Username</label>
                <input type="text" id="username" value="admin" required>
            </div>
            
            <div class="form-group">
                <label for="adminPassword">Admin Password</label>
                <input type="password" id="adminPassword" value="admin" required>
            </div>
            
            <button type="submit" class="btn" id="submitBtn">Connect</button>
        </form>
    </div>
    
    <script>
        // Scan networks on load
        fetch('/api/scan')
            .then(r => r.json())
            .then(networks => {
                const select = document.getElementById('ssid');
                select.innerHTML = '<option value="">-- Select Network --</option>';
                networks.forEach(n => {
                    const bars = Math.min(4, Math.max(1, Math.floor((n.rssi + 100) / 12)));
                    select.innerHTML += `<option value="${n.ssid}">${n.ssid} ${'▂'.repeat(bars)}</option>`;
                });
            })
            .catch(() => {
                document.getElementById('ssid').innerHTML = '<option value="">Scan failed</option>';
            });
        
        // Handle form submission
        document.getElementById('setupForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const btn = document.getElementById('submitBtn');
            const msg = document.getElementById('message');
            
            btn.disabled = true;
            btn.innerHTML = '<div class="spinner"></div>Connecting...';
            msg.style.display = 'none';
            
            const formData = new FormData();
            formData.append('ssid', document.getElementById('ssid').value);
            formData.append('password', document.getElementById('password').value);
            formData.append('username', document.getElementById('username').value);
            formData.append('adminPassword', document.getElementById('adminPassword').value);
            
            fetch('/api/configure', {
                method: 'POST',
                body: formData
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    msg.className = 'message success';
                    msg.textContent = data.message + ' Rebooting...';
                    msg.style.display = 'block';
                } else {
                    msg.className = 'message error';
                    msg.textContent = data.error;
                    msg.style.display = 'block';
                    btn.disabled = false;
                    btn.innerHTML = 'Connect';
                }
            })
            .catch(() => {
                msg.className = 'message error';
                msg.textContent = 'Connection error. Please try again.';
                msg.style.display = 'block';
                btn.disabled = false;
                btn.innerHTML = 'Connect';
            });
        });
    </script>
</body>
</html>
)rawliteral";
}
```

#### **8.3 Factory Reset Button (GPIO 19)**

Create `factory_reset.h`:

```cpp
#ifndef __FACTORY_RESET_H__
#define __FACTORY_RESET_H__

#include <Arduino.h>
#include "camera_settings.h"

class FactoryReset {
private:
    static constexpr uint8_t RESET_PIN = 19;
    static constexpr uint8_t LED_PIN = 2;  // Onboard LED (adjust if different)
    static constexpr unsigned long HOLD_TIME_MS = 10000;  // 10 seconds
    
    CameraSettings* settings;
    unsigned long pressStartTime;
    bool buttonPressed;
    
public:
    FactoryReset(CameraSettings* settings);
    void begin();
    void loop();  // Call in main loop or task
    
private:
    void performReset();
    void flashLED(int times, int delayMs);
};

#endif
```

Create `factory_reset.cpp`:

```cpp
#include "factory_reset.h"

FactoryReset::FactoryReset(CameraSettings* settings) 
    : settings(settings), pressStartTime(0), buttonPressed(false) {}

void FactoryReset::begin() {
    pinMode(RESET_PIN, INPUT_PULLUP);  // Button pulls to GND when pressed
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void FactoryReset::loop() {
    bool currentState = (digitalRead(RESET_PIN) == LOW);  // Active LOW
    
    if (currentState && !buttonPressed) {
        // Button just pressed
        buttonPressed = true;
        pressStartTime = millis();
        Serial.println("Reset button pressed...");
    } 
    else if (currentState && buttonPressed) {
        // Button held down
        unsigned long heldTime = millis() - pressStartTime;
        
        if (heldTime >= HOLD_TIME_MS) {
            Serial.println("Factory reset triggered!");
            performReset();
            buttonPressed = false;  // Prevent retriggering
        }
    }
    else if (!currentState && buttonPressed) {
        // Button released before hold time
        buttonPressed = false;
        Serial.println("Reset button released (too short)");
    }
}

void FactoryReset::performReset() {
    // Flash LED 3 times
    flashLED(3, 200);
    
    // Reset settings
    Serial.println("Resetting all settings to defaults...");
    settings->resetToDefault();
    settings->setWiFiConfigured(false);  // Trigger AP mode on next boot
    
    Serial.println("Rebooting...");
    delay(1000);
    ESP.restart();
}

void FactoryReset::flashLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}
```

#### **8.4 mDNS Support**

Add to main `.ino` file after WiFi connects:

```cpp
#include <ESPmDNS.h>

void setup() {
    // ... existing setup ...
    
    // After WiFi connection succeeds:
    if (WiFi.status() == WL_CONNECTED) {
        // Start mDNS
        if (MDNS.begin(settings->mdnsHostname)) {  // "camera" -> http://camera.local
            Serial.println("mDNS started: http://" + String(settings->mdnsHostname) + ".local");
            MDNS.addService("http", "tcp", 80);
        }
    }
}
```

#### **8.5 OTA Updates**

```cpp
#include <ArduinoOTA.h>

void setup() {
    // ... after WiFi connection ...
    
    // Configure OTA
    ArduinoOTA.setHostname(settings->mdnsHostname);
    ArduinoOTA.setPassword(settings->password);  // Use admin password
    
    ArduinoOTA.onStart([]() {
        Serial.println("OTA Update starting...");
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA Update complete!");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
    });
    
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();
}
```

#### **8.6 LED Status Indicators**

Create `led_status.h`:

```cpp
#ifndef __LED_STATUS_H__
#define __LED_STATUS_H__

#include <Arduino.h>

enum class LEDStatus {
    WIFI_CONNECTING,    // Slow blink
    WIFI_CONNECTED,     // Solid on
    AP_MODE,           // Slow double-blink
    STREAMING,         // Fast blink
    ERROR              // Fast triple-blink
};

class LEDStatusIndicator {
private:
    static constexpr uint8_t LED_PIN = 2;
    LEDStatus currentStatus;
    unsigned long lastUpdate;
    int blinkState;
    int blinkCount;
    
public:
    LEDStatusIndicator();
    void begin();
    void setStatus(LEDStatus status);
    void loop();  // Call frequently in loop/task
};

#endif
```

#### **8.7 Main .ino Integration**

Update main file to check WiFi configuration on boot:

```cpp
#include "wifi_provisioning.h"
#include "factory_reset.h"

WiFiProvisioning* provisioning = nullptr;
FactoryReset* resetHandler = nullptr;

void setup() {
    Serial.begin(115200);
    
    // Initialize settings
    settings = new CameraSettings();
    if (!settings->isNVSInitialized()) {
        settings->initializeNVS();
    }
    settings->readFromNVS();
    
    // Initialize factory reset handler
    resetHandler = new FactoryReset(settings);
    resetHandler->begin();
    
    // Check if WiFi is configured
    if (!settings->checkWiFiConfigured()) {
        Serial.println("WiFi not configured. Starting AP mode...");
        provisioning = new WiFiProvisioning(settings);
        provisioning->startAPMode();
        
        // Stay in provisioning loop
        while (provisioning->isActive()) {
            provisioning->handleDNS();
            resetHandler->loop();  // Allow factory reset during provisioning
            delay(10);
        }
    } else {
        // Normal station mode
        connectWiFi();
        
        // Start mDNS and OTA
        MDNS.begin(settings->mdnsHostname);
        ArduinoOTA.begin();
        
        // Initialize camera and web server
        initCamera();
        startWebServer();
    }
}

void loop() {
    if (provisioning && provisioning->isActive()) {
        provisioning->handleDNS();
    } else {
        ArduinoOTA.handle();
    }
    
    resetHandler->loop();
    delay(10);
}
```

### **Phase 9: Testing & Documentation**
- Test all endpoints with Postman or curl
- Test multiple browser connections
- Monitor serial output for errors
- Test settings persistence with power cycles
- Test Python clients (if implemented)
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
