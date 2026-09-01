/*
 * ESP32-S3 + OV2640 视频流推送到 Python 服务器
 * 
 * 功能：
 *   - 连接 WiFi
 *   - 初始化 OV2640 摄像头 (800x600 JPEG)
 *   - 每帧通过 HTTP POST 推送到 Python 服务器
 *   - 服务器可下发命令：拍照 / 开始录像 / 停止录像 / 切换分辨率
 *
 * 使用方法：
 *   1. 通过 CameraSettings/NVS 配置 WiFi 和 Python 服务器参数
 *   2. Arduino IDE 中选择 ESP32S3 开发板，启用 PSRAM (Octal)
 *   3. 上传本 sketch
 *
 * 引脚定义根据用户提供的丝印图修改
 */

#include "esp_camera.h"
#include <WiFi.h>
#include "camera_settings.h"
#include "web_server.h"
#include "factory_reset.h"
#include "wifi_provisioning.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <cstdlib>
#include <cstring>

// Set to 1 temporarily to run the Phase 1 persistence test. Set back to 0
// before normal camera operation. Reflashing does not erase NVS contents.
#define NVS_TEST_WRITE 0

// ==================== 摄像头引脚（按用户丝印图） ====================
#define PWDN_GPIO_NUM     15   // PWON
#define RESET_GPIO_NUM    16   // RST
#define XCLK_GPIO_NUM     -1
#define SIOD_GPIO_NUM     2    // SDA
#define SIOC_GPIO_NUM     1    // SCL
#define Y9_GPIO_NUM       14   // D7(DZ)
#define Y8_GPIO_NUM       13   // D6
#define Y7_GPIO_NUM       12   // D5
#define Y6_GPIO_NUM       11   // D4
#define Y5_GPIO_NUM       10   // D3
#define Y4_GPIO_NUM        9   // D2
#define Y3_GPIO_NUM        8   // D1
#define Y2_GPIO_NUM        7   // D0
#define VSYNC_GPIO_NUM     3   // VSYNC
#define HREF_GPIO_NUM      4   // HREF
#define PCLK_GPIO_NUM      5   // DCLK

// ==================== LED 反馈引脚 ====================
#define LED_GPIO_NUM       48  // 板载 LED（根据模块调整）
#define FACTORY_RESET_GPIO_NUM 19

// ==================== 全局变量 ====================

static CameraSettings settings;
static CameraWebServer webServer(80, &settings);
static framesize_t currentResolution = FRAMESIZE_SVGA;
static int currentQuality = 12;
static SemaphoreHandle_t cameraMutex;
static SemaphoreHandle_t frameMutex;
static uint8_t* latestFrame;
static size_t latestFrameLength;
static const size_t FRAME_BUFFER_CAPACITY = 512 * 1024;
static TaskHandle_t cameraTaskHandle;
static TaskHandle_t networkTaskHandle;
static FactoryReset factoryReset(&settings, FACTORY_RESET_GPIO_NUM, LED_GPIO_NUM);
static WiFiProvisioning provisioning(&settings);
bool reinitCamera(framesize_t resolution, int quality);

static bool frameSizeForResolutionSetting(uint8_t resolution, framesize_t& frameSize) {
    switch (resolution) {
        case 8:  frameSize = FRAMESIZE_QVGA; return true;
        case 9:  frameSize = FRAMESIZE_VGA;  return true;
        case 10: frameSize = FRAMESIZE_SVGA; return true;
        case 11: frameSize = FRAMESIZE_XGA;  return true;
        case 12: frameSize = FRAMESIZE_UXGA; return true;
        default: return false;
    }
}

static size_t captureJpeg(uint8_t* destination, size_t capacity) {
    if (!destination || capacity == 0 || !frameMutex || !latestFrame) return 0;
    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(1000)) != pdTRUE) return 0;
    const size_t length = latestFrameLength <= capacity ? latestFrameLength : 0;
    if (length > 0) memcpy(destination, latestFrame, length);
    xSemaphoreGive(frameMutex);
    return length;
}

static bool applyCameraConfig(uint8_t resolution, uint8_t quality, int8_t brightness,
                              int8_t contrast, int8_t saturation, bool verticalFlip,
                              bool horizontalMirror) {
    framesize_t requestedFrameSize;
    if (!frameSizeForResolutionSetting(resolution, requestedFrameSize)) return false;
    if (!cameraMutex) return false;
    if (xSemaphoreTake(cameraMutex, pdMS_TO_TICKS(3000)) != pdTRUE) return false;
    const bool needsReinit = currentResolution != requestedFrameSize ||
                             currentQuality != quality;
    settings.brightness = brightness;
    settings.contrast = contrast;
    settings.saturation = saturation;
    settings.verticalFlip = verticalFlip;
    settings.horizontalMirror = horizontalMirror;
    if (needsReinit && !reinitCamera(requestedFrameSize, quality)) {
        xSemaphoreGive(cameraMutex);
        return false;
    }
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor) {
        xSemaphoreGive(cameraMutex);
        return false;
    }
    sensor->set_vflip(sensor, verticalFlip ? 1 : 0);
    sensor->set_hmirror(sensor, horizontalMirror ? 1 : 0);
    sensor->set_brightness(sensor, brightness);
    sensor->set_contrast(sensor, contrast);
    sensor->set_saturation(sensor, saturation);
    xSemaphoreGive(cameraMutex);
    return true;
}

static void cameraTask(void*) {
    for (;;) {
        const uint8_t fps = settings.frameRate == 0 ? 10 : settings.frameRate;
        if (!cameraMutex || !frameMutex || !latestFrame) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (xSemaphoreTake(cameraMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            camera_fb_t* fb = esp_camera_fb_get();
            if (fb) {
                if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    const size_t length = fb->len <= FRAME_BUFFER_CAPACITY ? fb->len : 0;
                    if (length > 0) {
                        memcpy(latestFrame, fb->buf, length);
                        latestFrameLength = length;
                    }
                    xSemaphoreGive(frameMutex);
                }
                esp_camera_fb_return(fb);
            }
            xSemaphoreGive(cameraMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000UL / fps));
    }
}

static void networkTask(void*) {
    unsigned long lastWifiAttempt = 0;
    for (;;) {
        webServer.loop();
        if (settings.wifiSSID[0] != '\0' && WiFi.status() != WL_CONNECTED &&
            static_cast<unsigned long>(millis() - lastWifiAttempt) >= 10000UL) {
            lastWifiAttempt = millis();
            connectWiFi();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ==================== 摄像头初始化 ====================
bool initCamera(framesize_t resolution = FRAMESIZE_SVGA, int quality = 12) {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_count     = 2;       // 双缓冲提高帧率
    config.jpeg_quality = quality;
    config.frame_size   = resolution;

    // 检查 PSRAM
    if (psramFound()) {
        config.fb_count = 2;
        Serial0.println("[CAM] PSRAM found, using 2 frame buffers");
    } else {
        Serial0.println("[CAM] WARNING: PSRAM not found!");
        config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial0.printf("[CAM] Init FAILED: 0x%x\n", err);
        return false;
    }
    
    // 设置传感器参数
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_vflip(s, settings.verticalFlip ? 1 : 0);
        s->set_hmirror(s, settings.horizontalMirror ? 1 : 0);
        s->set_brightness(s, settings.brightness);
        s->set_contrast(s, settings.contrast);
        s->set_saturation(s, settings.saturation);
    }
    
    Serial0.printf("[CAM] Init OK | Resolution: %d | Quality: %d\n", resolution, quality);
    return true;
}

// ==================== 重新初始化摄像头 ====================
bool reinitCamera(framesize_t resolution, int quality) {
    Serial0.printf("[CAM] Reinit: resolution=%d, quality=%d\n", resolution, quality);
    
    // 释放当前摄像头
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK) {
        Serial0.printf("[CAM] Deinit FAILED: 0x%x\n", err);
        return false;
    }
    
    delay(100);  // 等待硬件稳定
    
    // 重新初始化
    if (!initCamera(resolution, quality)) {
        Serial0.println("[CAM] Reinit FAILED");
        return false;
    }
    
    currentResolution = resolution;
    currentQuality = quality;
    Serial0.println("[CAM] Reinit OK");
    return true;
}

// ==================== LED 闪烁反馈 ====================
void flashLED(int times = 2, int delayMs = 100) {
    pinMode(LED_GPIO_NUM, OUTPUT);
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_GPIO_NUM, HIGH);
        delay(delayMs);
        digitalWrite(LED_GPIO_NUM, LOW);
        delay(delayMs);
    }
}

// ==================== WiFi 连接 ====================
void connectWiFi() {
    if (settings.wifiSSID[0] == '\0') {
        Serial0.println("[WIFI] No SSID configured");
        return;
    }

    Serial0.printf("[WIFI] Connecting to %s", settings.wifiSSID);
    // Keep the station configuration intact while recovering from transient
    // disconnects. Erasing it on every retry can make reconnection unstable.
    WiFi.disconnect(false);
    delay(100);
    WiFi.mode(WIFI_STA);
    if (settings.useDHCP) {
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    } else {
        IPAddress ip(settings.staticIP[0], settings.staticIP[1], settings.staticIP[2], settings.staticIP[3]);
        IPAddress gateway(settings.gateway[0], settings.gateway[1], settings.gateway[2], settings.gateway[3]);
        IPAddress subnet(settings.subnet[0], settings.subnet[1], settings.subnet[2], settings.subnet[3]);
        if (!WiFi.config(ip, gateway, subnet)) {
            Serial0.println("\n[WIFI] Failed to apply static network settings");
        }
    }
    WiFi.begin(settings.wifiSSID, settings.wifiPassword);
    WiFi.setSleep(false);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial0.print(".");
        if (++retries > 40) {
            Serial0.println("\n[WIFI] Failed");
            return;
        }
    }
    Serial0.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

// ==================== 解析服务器命令 ====================
void handleCommand(const String& resp) {
    // 简单的 JSON 解析，查找 "cmd" 字段
    int cmdIdx = resp.indexOf("\"cmd\"");
    if (cmdIdx < 0) return;
    
    // 解析命令类型
    if (resp.indexOf("\"photo\"") >= 0) {
        Serial0.println("[CMD] photo -> LED flash");
        flashLED(3, 80);
    }
    else if (resp.indexOf("\"set_resolution\"") >= 0) {
        // 解析分辨率值
        int valIdx = resp.indexOf("\"value\"");
        if (valIdx >= 0) {
            uint8_t newResolution = settings.cameraResolution;
            int newQuality = currentQuality;

            if (resp.indexOf("\"QVGA\"") >= 0) newResolution = 8;
            else if (resp.indexOf("\"VGA\"") >= 0) newResolution = 9;
            else if (resp.indexOf("\"SVGA\"") >= 0) newResolution = 10;
            else if (resp.indexOf("\"XGA\"") >= 0) newResolution = 11;
            else if (resp.indexOf("\"UXGA\"") >= 0) newResolution = 12;
            
            // 解析质量值
            int qIdx = resp.indexOf("\"quality\"");
            if (qIdx >= 0) {
                int start = resp.indexOf(":", qIdx) + 1;
                int end = resp.indexOf(",", start);
                if (end < 0) end = resp.indexOf("}", start);
                if (start > 0 && end > start) {
                    newQuality = resp.substring(start, end).toInt();
                    if (newQuality < 10) newQuality = 10;
                    if (newQuality > 63) newQuality = 63;
                }
            }
            
            framesize_t newFrameSize;
            if (!frameSizeForResolutionSetting(newResolution, newFrameSize)) return;
            Serial0.printf("[CMD] set_resolution: setting=%u, frame_size=%d, quality=%d\n",
                           newResolution, newFrameSize, newQuality);
            if (reinitCamera(newFrameSize, newQuality)) {
                settings.writeCameraResolution(newResolution);
                settings.writeCameraQuality(static_cast<uint8_t>(newQuality));
            }
        }
    }
    else if (resp.indexOf("\"get_status\"") >= 0) {
        Serial0.printf("[CMD] get_status: res=%d, quality=%d\n", currentResolution, currentQuality);
    }
}

// ==================== setup ====================
void setup() {
    Serial0.begin(115200);
    Serial0.println("\n===========================");
    Serial0.println(" ESP32-S3 Camera Streamer");
    Serial0.println("===========================");

    // 初始化 LED
    pinMode(LED_GPIO_NUM, OUTPUT);
    digitalWrite(LED_GPIO_NUM, LOW);

    if (!settings.isNVSInitialized()) {
        Serial0.println("[SETTINGS] First boot - initializing NVS");
        if (!settings.initializeNVS()) {
            Serial0.println("[SETTINGS] NVS initialization failed; using defaults");
        }
    }
    settings.readFromNVS();
    settings.printSettings();
    factoryReset.begin();

    if (!settings.checkWiFiConfigured()) {
        Serial0.println("[PROVISION] WiFi not configured; entering AP mode");
        if (provisioning.startAPMode()) {
            while (provisioning.isActive() && !provisioning.timedOut()) {
                provisioning.handleDNS();
                factoryReset.loop();
                delay(10);
            }
            if (provisioning.isActive()) {
                Serial0.println("[PROVISION] AP timeout; trying Station mode");
                provisioning.stopAPMode();
            }
        }
    }

#if NVS_TEST_WRITE
    Serial0.println("[NVS TEST] Writing test values...");
    Serial0.printf("WiFi settings: %d\n",
                   settings.writeWiFiSettings(CameraSettings::DefaultValues::WIFI_SSID,
                                              strlen(CameraSettings::DefaultValues::WIFI_SSID),
                                              CameraSettings::DefaultValues::WIFI_PASSWORD,
                                              strlen(CameraSettings::DefaultValues::WIFI_PASSWORD)));
    Serial0.printf("deviceName: %d\n",
                   settings.writeDeviceName("PersistTest", 11));
    Serial0.printf("cameraQuality: %d\n",
                   settings.writeCameraQuality(25));
    Serial0.printf("frameRate: %d\n",
                   settings.writeFrameRate(10));
    Serial0.printf("brightness: %d\n",
                   settings.writeBrightness(2));
    Serial0.printf("invalid quality rejected: %d\n",
                   !settings.writeCameraQuality(9));

    settings.readFromNVS();
    settings.printSettings();
    Serial0.println("[NVS TEST] Write phase complete.");
    Serial0.println("Set NVS_TEST_WRITE to 0, reflash, and check that these values remain.");
    while (true) {
        delay(1000);
    }
#endif

    // Translate stable API/NVS values to the enum used by this camera library.
    if (!frameSizeForResolutionSetting(settings.cameraResolution, currentResolution)) {
        settings.cameraResolution = CameraSettings::DefaultValues::CAMERA_RESOLUTION;
        frameSizeForResolutionSetting(settings.cameraResolution, currentResolution);
    }
    if (settings.cameraQuality < 10 || settings.cameraQuality > 63) {
        settings.cameraQuality = CameraSettings::DefaultValues::CAMERA_QUALITY;
    }
    if (settings.frameRate != 5 && settings.frameRate != 10 &&
        settings.frameRate != 15 && settings.frameRate != 20) {
        settings.frameRate = CameraSettings::DefaultValues::FRAME_RATE;
    }
    currentQuality = settings.cameraQuality;

    if (!initCamera(currentResolution, currentQuality)) {
        Serial0.println("Camera init failed, halting.");
        while (true) delay(1000);
    }

    cameraMutex = xSemaphoreCreateMutex();
    frameMutex = xSemaphoreCreateMutex();
    latestFrame = static_cast<uint8_t*>(ps_malloc(FRAME_BUFFER_CAPACITY));
    if (!cameraMutex || !frameMutex || !latestFrame) {
        Serial0.println("[SYS] Failed to allocate camera task resources");
        while (true) delay(1000);
    }

    webServer.setReconnectCallback([]() {
        connectWiFi();
    });
    webServer.setFrameCaptureCallback(captureJpeg);
    webServer.setFrameRateCallback([]() { return settings.frameRate; });
    webServer.setCameraConfigCallback(applyCameraConfig);
    connectWiFi();

    // Starting while disconnected lets the server become reachable after a
    // later automatic reconnect without requiring a reboot.
    webServer.begin();
    xTaskCreatePinnedToCore(cameraTask, "CameraCapture", 8192, nullptr, 2,
                            &cameraTaskHandle, 0);
    xTaskCreatePinnedToCore(networkTask, "NetworkService", 8192, nullptr, 1,
                            &networkTaskHandle, 1);

    // 启动时闪烁 LED 表示就绪
    flashLED(1, 200);
    
    Serial0.println("[SYS] Starting stream loop...");
}

// ==================== loop ====================
void loop() {
    factoryReset.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
}
