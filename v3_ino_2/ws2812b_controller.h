#ifndef WS2812B_CONTROLLER_H
#define WS2812B_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class Ws2812bController {
public:
    static constexpr uint16_t MAX_LEDS = 45;
    static constexpr uint16_t POWER_BUDGET_MA = 3000;
    static constexpr uint8_t CURRENT_PER_LED_MA = 60;

    struct State {
        bool initialized = false;
        bool enabled = false;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        uint8_t brightnessPercent = 0;
        uint8_t effectiveBrightnessPercent = 0;
        uint16_t count = 0;
        uint8_t dataPin = 0;
    };

    Ws2812bController();
    ~Ws2812bController();

    bool begin(uint16_t count, uint8_t dataPin, bool enabled,
               uint8_t red, uint8_t green, uint8_t blue,
               uint8_t brightnessPercent);
    bool reinitialize(uint16_t count, uint8_t dataPin);
    bool apply(bool enabled, uint8_t red, uint8_t green, uint8_t blue,
               uint8_t brightnessPercent);
    // Blocking flash used for short boot/reset status indications.
    bool flash(uint8_t times, unsigned long durationMs,
               uint8_t red, uint8_t green, uint8_t blue);
    State state() const;

    static bool isValidCount(uint16_t count);
    static bool isValidDataPin(uint8_t dataPin);
    static bool isValidIndicatorPin(uint8_t dataPin);
    static uint8_t effectiveBrightness(uint16_t count, uint8_t requestedPercent);

private:
    Adafruit_NeoPixel* strip_;
    mutable SemaphoreHandle_t mutex_;
    State state_;

    bool applyLocked();
    bool reinitializeLocked(uint16_t count, uint8_t dataPin);
};

#endif
