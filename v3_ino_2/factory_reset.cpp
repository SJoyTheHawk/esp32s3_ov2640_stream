#include "factory_reset.h"

FactoryReset::FactoryReset(CameraSettings* settings, uint8_t resetPin,
                           Ws2812bController* indicator)
    : settings_(settings), resetPin_(resetPin), indicator_(indicator), rawPressed_(false),
      stablePressed_(false), resetTriggered_(false),
      rawChangedAtMs_(0), pressedAtMs_(0), lastCountdownSecond_(0) {}

void FactoryReset::begin() {
    pinMode(resetPin_, INPUT_PULLUP);
    rawPressed_ = stablePressed_ = (digitalRead(resetPin_) == LOW);
    rawChangedAtMs_ = millis();
    pressedAtMs_ = rawPressed_ ? rawChangedAtMs_ : 0;
    lastCountdownSecond_ = 0;
    if (rawPressed_) {
        Serial0.printf("[RESET] Factory reset button pressed on GPIO %u; reset in %lu seconds if held\n",
                       resetPin_, HOLD_TIME_MS / 1000UL);
    }
}

void FactoryReset::loop() {
    const unsigned long now = millis();
    const bool pressed = digitalRead(resetPin_) == LOW;
    if (pressed != rawPressed_) { rawPressed_ = pressed; rawChangedAtMs_ = now; }
    if (rawPressed_ != stablePressed_ && now - rawChangedAtMs_ >= DEBOUNCE_MS) {
        stablePressed_ = rawPressed_;
        if (stablePressed_) {
            pressedAtMs_ = now;
            resetTriggered_ = false;
            lastCountdownSecond_ = 0;
            Serial0.printf("[RESET] Factory reset button pressed on GPIO %u; reset in %lu seconds if held\n",
                           resetPin_, HOLD_TIME_MS / 1000UL);
        } else {
            pressedAtMs_ = 0;
            resetTriggered_ = false;
            lastCountdownSecond_ = 0;
            Serial0.println("[RESET] Factory reset button released; reset cancelled");
        }
    }
    if (stablePressed_ && !resetTriggered_ && pressedAtMs_ != 0) {
        const unsigned long elapsedMs = now - pressedAtMs_;
        if (elapsedMs >= HOLD_TIME_MS) {
            resetTriggered_ = true;
            performReset();
        } else {
            const unsigned long elapsedSecond = elapsedMs / 1000UL;
            if (elapsedSecond > lastCountdownSecond_) {
                lastCountdownSecond_ = elapsedSecond;
                const unsigned long remainingMs = HOLD_TIME_MS - elapsedMs;
                const unsigned long remainingSeconds = (remainingMs + 999UL) / 1000UL;
                Serial0.printf("[RESET] Factory reset in %lu %s\n", remainingSeconds,
                               remainingSeconds == 1 ? "second" : "seconds");
                if (indicator_) indicator_->flash(1, 200, 255, 0, 0);
            }
        }
    }
}

void FactoryReset::performReset() {
    Serial0.println("[RESET] Factory reset triggered");
    flashLED(5, 200);
    if (settings_) { settings_->resetToDefault(); settings_->setWiFiConfigured(false); }
    Serial0.println("[RESET] Restarting");
    delay(500);
    ESP.restart();
}

void FactoryReset::flashLED(uint8_t times, unsigned long durationMs) {
    if (!indicator_) return;
    indicator_->flash(times, durationMs, 255, 0, 0);
}
