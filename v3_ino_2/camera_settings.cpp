#include "camera_settings.h"

#include <cstring>

const char* const CameraSettings::NVS_NAMESPACE = "camera";

// Out-of-class definitions keep the string and byte constexpr members linkable
// with the C++11 toolchain used by many Arduino ESP32 installations.
constexpr char CameraSettings::DefaultValues::USERNAME[32];
constexpr char CameraSettings::DefaultValues::PASSWORD[32];
constexpr byte CameraSettings::DefaultValues::STATIC_IP[4];
constexpr byte CameraSettings::DefaultValues::GATEWAY[4];
constexpr byte CameraSettings::DefaultValues::SUBNET[4];
constexpr char CameraSettings::DefaultValues::WIFI_SSID[32];
constexpr char CameraSettings::DefaultValues::WIFI_PASSWORD[32];
constexpr char CameraSettings::DefaultValues::PYTHON_SERVER_IP[32];
constexpr char CameraSettings::DefaultValues::DEVICE_NAME[32];
constexpr char CameraSettings::DefaultValues::MDNS_HOSTNAME[32];

CameraSettings::CameraSettings() {
    setDefaults();
}

void CameraSettings::setDefaults() {
    strncpy(username, DefaultValues::USERNAME, sizeof(username));
    strncpy(password, DefaultValues::PASSWORD, sizeof(password));
    username[sizeof(username) - 1] = '\0';
    password[sizeof(password) - 1] = '\0';

    useDHCP = DefaultValues::USE_DHCP;
    memcpy(staticIP, DefaultValues::STATIC_IP, sizeof(staticIP));
    memcpy(gateway, DefaultValues::GATEWAY, sizeof(gateway));
    memcpy(subnet, DefaultValues::SUBNET, sizeof(subnet));

    strncpy(wifiSSID, DefaultValues::WIFI_SSID, sizeof(wifiSSID));
    strncpy(wifiPassword, DefaultValues::WIFI_PASSWORD, sizeof(wifiPassword));
    wifiSSID[sizeof(wifiSSID) - 1] = '\0';
    wifiPassword[sizeof(wifiPassword) - 1] = '\0';

    cameraResolution = DefaultValues::CAMERA_RESOLUTION;
    cameraQuality = DefaultValues::CAMERA_QUALITY;
    frameRate = DefaultValues::FRAME_RATE;
    brightness = DefaultValues::BRIGHTNESS;
    contrast = DefaultValues::CONTRAST;
    saturation = DefaultValues::SATURATION;
    verticalFlip = DefaultValues::VERTICAL_FLIP;
    horizontalMirror = DefaultValues::HORIZONTAL_MIRROR;

    pythonServerEnabled = DefaultValues::PYTHON_SERVER_ENABLED;
    strncpy(pythonServerIP, DefaultValues::PYTHON_SERVER_IP, sizeof(pythonServerIP));
    pythonServerIP[sizeof(pythonServerIP) - 1] = '\0';
    pythonServerPort = DefaultValues::PYTHON_SERVER_PORT;

    strncpy(deviceName, DefaultValues::DEVICE_NAME, sizeof(deviceName));
    strncpy(mdnsHostname, DefaultValues::MDNS_HOSTNAME, sizeof(mdnsHostname));
    deviceName[sizeof(deviceName) - 1] = '\0';
    mdnsHostname[sizeof(mdnsHostname) - 1] = '\0';
}

bool CameraSettings::isNVSInitialized() {
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    const bool initialized = prefs.getBool("initialized", false);
    prefs.end();
    return initialized;
}

bool CameraSettings::initializeNVS() {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("[SETTINGS] Failed to open NVS for writing");
        return false;
    }

    bool ok = true;
    prefs.putString("username", DefaultValues::USERNAME);
    prefs.putString("password", DefaultValues::PASSWORD);
    ok &= prefs.putBool("useDHCP", DefaultValues::USE_DHCP);
    ok &= prefs.putBytes("staticIP", DefaultValues::STATIC_IP, sizeof(staticIP)) == sizeof(staticIP);
    ok &= prefs.putBytes("gateway", DefaultValues::GATEWAY, sizeof(gateway)) == sizeof(gateway);
    ok &= prefs.putBytes("subnet", DefaultValues::SUBNET, sizeof(subnet)) == sizeof(subnet);
    prefs.putString("wifiSSID", DefaultValues::WIFI_SSID);
    prefs.putString("wifiPass", DefaultValues::WIFI_PASSWORD);
    ok &= prefs.putUChar("camRes", DefaultValues::CAMERA_RESOLUTION) > 0;
    ok &= prefs.putUChar("camQual", DefaultValues::CAMERA_QUALITY) > 0;
    ok &= prefs.putUChar("frameRate", DefaultValues::FRAME_RATE) > 0;
    ok &= prefs.putChar("brightness", DefaultValues::BRIGHTNESS) > 0;
    ok &= prefs.putChar("contrast", DefaultValues::CONTRAST) > 0;
    ok &= prefs.putChar("saturation", DefaultValues::SATURATION) > 0;
    ok &= prefs.putBool("vFlip", DefaultValues::VERTICAL_FLIP);
    ok &= prefs.putBool("hMirror", DefaultValues::HORIZONTAL_MIRROR);
    ok &= prefs.putBool("pyEnabled", DefaultValues::PYTHON_SERVER_ENABLED);
    prefs.putString("pyIP", DefaultValues::PYTHON_SERVER_IP);
    ok &= prefs.putUShort("pyPort", DefaultValues::PYTHON_SERVER_PORT) > 0;
    prefs.putString("deviceName", DefaultValues::DEVICE_NAME);
    prefs.putString("mdnsHost", DefaultValues::MDNS_HOSTNAME);
    ok &= prefs.isKey("username") && prefs.isKey("password") &&
          prefs.isKey("wifiSSID") && prefs.isKey("wifiPass") &&
          prefs.isKey("pyIP") && prefs.isKey("deviceName") && prefs.isKey("mdnsHost");
    if (ok) {
        ok &= prefs.putBool("initialized", true);
    }
    prefs.end();

    if (ok) {
        setDefaults();
        Serial.println("[SETTINGS] NVS initialized successfully");
    } else {
        Serial.println("[SETTINGS] Failed to write NVS defaults");
    }
    return ok;
}

bool CameraSettings::resetToDefault() {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    const bool cleared = prefs.clear();
    prefs.end();
    return cleared && initializeNVS();
}

void CameraSettings::readFromNVS() {
    setDefaults();
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        Serial.println("[SETTINGS] Failed to open NVS for reading; using defaults");
        return;
    }

    String value = prefs.getString("username", DefaultValues::USERNAME);
    value.toCharArray(username, sizeof(username));
    value = prefs.getString("password", DefaultValues::PASSWORD);
    value.toCharArray(password, sizeof(password));

    useDHCP = prefs.getBool("useDHCP", DefaultValues::USE_DHCP);
    if (prefs.getBytesLength("staticIP") == sizeof(staticIP)) {
        prefs.getBytes("staticIP", staticIP, sizeof(staticIP));
    }
    if (prefs.getBytesLength("gateway") == sizeof(gateway)) {
        prefs.getBytes("gateway", gateway, sizeof(gateway));
    }
    if (prefs.getBytesLength("subnet") == sizeof(subnet)) {
        prefs.getBytes("subnet", subnet, sizeof(subnet));
    }

    value = prefs.getString("wifiSSID", DefaultValues::WIFI_SSID);
    value.toCharArray(wifiSSID, sizeof(wifiSSID));
    value = prefs.getString("wifiPass", DefaultValues::WIFI_PASSWORD);
    value.toCharArray(wifiPassword, sizeof(wifiPassword));

    cameraResolution = prefs.getUChar("camRes", DefaultValues::CAMERA_RESOLUTION);
    cameraQuality = prefs.getUChar("camQual", DefaultValues::CAMERA_QUALITY);
    frameRate = prefs.getUChar("frameRate", DefaultValues::FRAME_RATE);
    brightness = prefs.getChar("brightness", DefaultValues::BRIGHTNESS);
    contrast = prefs.getChar("contrast", DefaultValues::CONTRAST);
    saturation = prefs.getChar("saturation", DefaultValues::SATURATION);
    verticalFlip = prefs.getBool("vFlip", DefaultValues::VERTICAL_FLIP);
    horizontalMirror = prefs.getBool("hMirror", DefaultValues::HORIZONTAL_MIRROR);

    pythonServerEnabled = prefs.getBool("pyEnabled", DefaultValues::PYTHON_SERVER_ENABLED);
    value = prefs.getString("pyIP", DefaultValues::PYTHON_SERVER_IP);
    value.toCharArray(pythonServerIP, sizeof(pythonServerIP));
    pythonServerPort = prefs.getUShort("pyPort", DefaultValues::PYTHON_SERVER_PORT);

    value = prefs.getString("deviceName", DefaultValues::DEVICE_NAME);
    value.toCharArray(deviceName, sizeof(deviceName));
    value = prefs.getString("mdnsHost", DefaultValues::MDNS_HOSTNAME);
    value.toCharArray(mdnsHostname, sizeof(mdnsHostname));
    prefs.end();

    // Migrate devices initialized by the previous phase-1 build, which stored
    // blank WiFi credentials because there was no configuration entry point.
    if (wifiSSID[0] == '\0' && DefaultValues::WIFI_SSID[0] != '\0') {
        strncpy(wifiSSID, DefaultValues::WIFI_SSID, sizeof(wifiSSID));
        strncpy(wifiPassword, DefaultValues::WIFI_PASSWORD, sizeof(wifiPassword));
        wifiSSID[sizeof(wifiSSID) - 1] = '\0';
        wifiPassword[sizeof(wifiPassword) - 1] = '\0';
        if (prefs.begin(NVS_NAMESPACE, false)) {
            prefs.putString("wifiSSID", wifiSSID);
            prefs.putString("wifiPass", wifiPassword);
            prefs.end();
        }
    }

    Serial.println("[SETTINGS] Loaded from NVS");
}

bool CameraSettings::writeStringSetting(const char* key, const char* value, size_t length,
                                        char* destination, size_t destinationSize) {
    if (!key || !value || length >= destinationSize ||
        strnlen(value, destinationSize) != length || !prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    prefs.putString(key, value);
    const bool ok = prefs.isKey(key);
    prefs.end();
    if (ok) {
        memcpy(destination, value, length);
        destination[length] = '\0';
    }
    return ok;
}

bool CameraSettings::writeUsername(const char* user, size_t length) {
    return writeStringSetting("username", user, length, username, sizeof(username));
}

bool CameraSettings::writePassword(const char* pass, size_t length) {
    return writeStringSetting("password", pass, length, password, sizeof(password));
}

bool CameraSettings::writeNetworkSettings(bool dhcp, const byte ip[4], const byte gw[4], const byte sn[4]) {
    if (!ip || !gw || !sn || !prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    const bool ok = prefs.putBool("useDHCP", dhcp) &&
                   prefs.putBytes("staticIP", ip, sizeof(staticIP)) == sizeof(staticIP) &&
                   prefs.putBytes("gateway", gw, sizeof(gateway)) == sizeof(gateway) &&
                   prefs.putBytes("subnet", sn, sizeof(subnet)) == sizeof(subnet);
    prefs.end();
    if (ok) {
        useDHCP = dhcp;
        memcpy(staticIP, ip, sizeof(staticIP));
        memcpy(gateway, gw, sizeof(gateway));
        memcpy(subnet, sn, sizeof(subnet));
    }
    return ok;
}

bool CameraSettings::writeDHCPSetting(bool dhcp) {
    if (!prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putBool("useDHCP", dhcp);
    prefs.end();
    if (ok) useDHCP = dhcp;
    return ok;
}

bool CameraSettings::writeStaticIP(const byte ip[4]) {
    if (!ip || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putBytes("staticIP", ip, sizeof(staticIP)) == sizeof(staticIP);
    prefs.end();
    if (ok) memcpy(staticIP, ip, sizeof(staticIP));
    return ok;
}

bool CameraSettings::writeGateway(const byte gw[4]) {
    if (!gw || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putBytes("gateway", gw, sizeof(gateway)) == sizeof(gateway);
    prefs.end();
    if (ok) memcpy(gateway, gw, sizeof(gateway));
    return ok;
}

bool CameraSettings::writeSubnet(const byte sn[4]) {
    if (!sn || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putBytes("subnet", sn, sizeof(subnet)) == sizeof(subnet);
    prefs.end();
    if (ok) memcpy(subnet, sn, sizeof(subnet));
    return ok;
}

bool CameraSettings::writeWiFiSettings(const char* ssid, size_t ssidLen,
                                       const char* pass, size_t passLen) {
    if (!ssid || !pass || ssidLen >= sizeof(wifiSSID) || passLen >= sizeof(wifiPassword) ||
        strnlen(ssid, sizeof(wifiSSID)) != ssidLen ||
        strnlen(pass, sizeof(wifiPassword)) != passLen ||
        !prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    prefs.putString("wifiSSID", ssid);
    prefs.putString("wifiPass", pass);
    const bool ok = prefs.isKey("wifiSSID") && prefs.isKey("wifiPass");
    prefs.end();
    if (ok) {
        memcpy(wifiSSID, ssid, ssidLen);
        wifiSSID[ssidLen] = '\0';
        memcpy(wifiPassword, pass, passLen);
        wifiPassword[passLen] = '\0';
    }
    return ok;
}

bool CameraSettings::writeSSID(const char* ssid, size_t length) {
    return writeStringSetting("wifiSSID", ssid, length, wifiSSID, sizeof(wifiSSID));
}

bool CameraSettings::writeWiFiPassword(const char* pass, size_t length) {
    return writeStringSetting("wifiPass", pass, length, wifiPassword, sizeof(wifiPassword));
}

bool CameraSettings::writeCameraResolution(uint8_t resolution) {
    if (resolution < 8 || resolution > 12 || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putUChar("camRes", resolution) > 0;
    prefs.end();
    if (ok) cameraResolution = resolution;
    return ok;
}

bool CameraSettings::writeCameraQuality(uint8_t quality) {
    if (quality < 10 || quality > 63 || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putUChar("camQual", quality) > 0;
    prefs.end();
    if (ok) cameraQuality = quality;
    return ok;
}

bool CameraSettings::writeFrameRate(uint8_t fps) {
    if ((fps != 5 && fps != 10 && fps != 15 && fps != 20) || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putUChar("frameRate", fps) > 0;
    prefs.end();
    if (ok) frameRate = fps;
    return ok;
}

bool CameraSettings::writeBrightness(int8_t value) {
    if (value < -2 || value > 2 || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putChar("brightness", value) > 0;
    prefs.end();
    if (ok) brightness = value;
    return ok;
}

bool CameraSettings::writeContrast(int8_t value) {
    if (value < -2 || value > 2 || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putChar("contrast", value) > 0;
    prefs.end();
    if (ok) contrast = value;
    return ok;
}

bool CameraSettings::writeSaturation(int8_t value) {
    if (value < -2 || value > 2 || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putChar("saturation", value) > 0;
    prefs.end();
    if (ok) saturation = value;
    return ok;
}

bool CameraSettings::writeVerticalFlip(bool flip) {
    if (!prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putBool("vFlip", flip);
    prefs.end();
    if (ok) verticalFlip = flip;
    return ok;
}

bool CameraSettings::writeHorizontalMirror(bool mirror) {
    if (!prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putBool("hMirror", mirror);
    prefs.end();
    if (ok) horizontalMirror = mirror;
    return ok;
}

bool CameraSettings::writePythonServerEnabled(bool enabled) {
    if (!prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putBool("pyEnabled", enabled);
    prefs.end();
    if (ok) pythonServerEnabled = enabled;
    return ok;
}

bool CameraSettings::writePythonServerIP(const char* ip, size_t length) {
    return writeStringSetting("pyIP", ip, length, pythonServerIP, sizeof(pythonServerIP));
}

bool CameraSettings::writePythonServerPort(uint16_t port) {
    if (port == 0 || !prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putUShort("pyPort", port) > 0;
    prefs.end();
    if (ok) pythonServerPort = port;
    return ok;
}

bool CameraSettings::writeDeviceName(const char* name, size_t length) {
    return writeStringSetting("deviceName", name, length, deviceName, sizeof(deviceName));
}

bool CameraSettings::writeMDNSHostname(const char* hostname, size_t length) {
    return writeStringSetting("mdnsHost", hostname, length, mdnsHostname, sizeof(mdnsHostname));
}

void CameraSettings::printSettings() {
    Serial.println("\n========== Camera Settings ==========");
    Serial.printf("Username: %s\n", username);
    Serial.println("Password: [HIDDEN]");
    Serial.printf("WiFi SSID: %s\n", wifiSSID);
    Serial.println("WiFi Password: [HIDDEN]");
    Serial.printf("Use DHCP: %s\n", useDHCP ? "Yes" : "No");
    Serial.printf("Static IP: %u.%u.%u.%u\n", staticIP[0], staticIP[1], staticIP[2], staticIP[3]);
    Serial.printf("Gateway: %u.%u.%u.%u\n", gateway[0], gateway[1], gateway[2], gateway[3]);
    Serial.printf("Subnet: %u.%u.%u.%u\n", subnet[0], subnet[1], subnet[2], subnet[3]);
    Serial.printf("Camera Resolution: %u\n", cameraResolution);
    Serial.printf("Camera Quality: %u\n", cameraQuality);
    Serial.printf("Frame Rate: %u fps\n", frameRate);
    Serial.printf("Brightness: %d\n", brightness);
    Serial.printf("Contrast: %d\n", contrast);
    Serial.printf("Saturation: %d\n", saturation);
    Serial.printf("Vertical Flip: %s\n", verticalFlip ? "Yes" : "No");
    Serial.printf("Horizontal Mirror: %s\n", horizontalMirror ? "Yes" : "No");
    Serial.printf("Python Server Enabled: %s\n", pythonServerEnabled ? "Yes" : "No");
    Serial.printf("Python Server: %s:%u\n", pythonServerIP, pythonServerPort);
    Serial.printf("Device Name: %s\n", deviceName);
    Serial.printf("mDNS Hostname: %s\n", mdnsHostname);
    Serial.println("=====================================\n");
}
