#ifndef CAMERA_SETTINGS_H
#define CAMERA_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

// Persistent configuration for the camera and its optional Python companion.
// All string fields are fixed-size buffers so they can be passed directly to
// the Arduino networking APIs without heap ownership concerns.
class CameraSettings {
public:
    struct DefaultValues {
        static constexpr char USERNAME[32] = "admin";
        static constexpr char PASSWORD[32] = "admin";

        static constexpr bool USE_DHCP = true;
        static constexpr byte STATIC_IP[4] = {192, 168, 2, 100};
        static constexpr byte GATEWAY[4] = {192, 168, 2, 1};
        static constexpr byte SUBNET[4] = {255, 255, 255, 0};

        static constexpr char WIFI_SSID[32] = "";
        static constexpr char WIFI_PASSWORD[32] = "";

        // OV2640 frame-size values: QVGA=8, VGA=9, SVGA=10, XGA=11, UXGA=12.
        static constexpr uint8_t CAMERA_RESOLUTION = 10;
        static constexpr uint8_t CAMERA_QUALITY = 12;
        static constexpr uint8_t FRAME_RATE = 20;
        static constexpr int8_t BRIGHTNESS = 0;
        static constexpr int8_t CONTRAST = 0;
        static constexpr int8_t SATURATION = 0;
        static constexpr bool VERTICAL_FLIP = false;
        static constexpr bool HORIZONTAL_MIRROR = false;

        static constexpr bool PYTHON_SERVER_ENABLED = false;
        static constexpr char PYTHON_SERVER_IP[32] = "192.168.1.183";
        static constexpr uint16_t PYTHON_SERVER_PORT = 8000;

        static constexpr char DEVICE_NAME[32] = "ESP32-Camera";
        static constexpr char MDNS_HOSTNAME[32] = "camera";
    };

    char username[32];
    char password[32];

    bool useDHCP;
    byte staticIP[4];
    byte gateway[4];
    byte subnet[4];

    char wifiSSID[32];
    char wifiPassword[32];

    uint8_t cameraResolution;
    uint8_t cameraQuality;
    uint8_t frameRate;
    int8_t brightness;
    int8_t contrast;
    int8_t saturation;
    bool verticalFlip;
    bool horizontalMirror;

    bool pythonServerEnabled;
    char pythonServerIP[32];
    uint16_t pythonServerPort;

    char deviceName[32];
    char mdnsHostname[32];

    CameraSettings();

    bool isNVSInitialized();
    bool initializeNVS();
    bool resetToDefault();
    void readFromNVS();

    bool writeUsername(const char* user, size_t length);
    bool writePassword(const char* pass, size_t length);

    bool writeNetworkSettings(bool dhcp, const byte ip[4], const byte gw[4], const byte sn[4]);
    bool writeDHCPSetting(bool dhcp);
    bool writeStaticIP(const byte ip[4]);
    bool writeGateway(const byte gw[4]);
    bool writeSubnet(const byte sn[4]);

    bool writeWiFiSettings(const char* ssid, size_t ssidLen,
                           const char* pass, size_t passLen);
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

    void printSettings();

private:
    Preferences prefs;
    static const char* const NVS_NAMESPACE;

    void setDefaults();
    bool writeStringSetting(const char* key, const char* value, size_t length,
                            char* destination, size_t destinationSize);
};

#endif // CAMERA_SETTINGS_H
