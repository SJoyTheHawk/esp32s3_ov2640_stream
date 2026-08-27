#ifndef FACTORY_RESET_H
#define FACTORY_RESET_H

#include <Arduino.h>
#include "camera_settings.h"

class FactoryReset {
public:
    FactoryReset(CameraSettings* settings, uint8_t resetPin, uint8_t ledPin);
    void begin();
    void loop();

private:
    static constexpr unsigned long DEBOUNCE_MS = 50;
    static constexpr unsigned long HOLD_TIME_MS = 10000;
    CameraSettings* settings_;
    uint8_t resetPin_;
    uint8_t ledPin_;
    bool rawPressed_;
    bool stablePressed_;
    bool resetTriggered_;
    unsigned long rawChangedAtMs_;
    unsigned long pressedAtMs_;
    void performReset();
    void flashLED(uint8_t times, unsigned long durationMs);
};

#endif
