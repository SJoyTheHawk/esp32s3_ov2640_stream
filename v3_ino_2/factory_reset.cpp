#include "factory_reset.h"

FactoryReset::FactoryReset(CameraSettings* settings, uint8_t resetPin, uint8_t ledPin)
    : settings_(settings), resetPin_(resetPin), ledPin_(ledPin), rawPressed_(false),
      stablePressed_(false), resetTriggered_(false), rawChangedAtMs_(0), pressedAtMs_(0) {}

void FactoryReset::begin() {
    pinMode(resetPin_, INPUT_PULLUP);
    pinMode(ledPin_, OUTPUT);
    digitalWrite(ledPin_, LOW);
    rawPressed_ = stablePressed_ = (digitalRead(resetPin_) == LOW);
    rawChangedAtMs_ = millis();
    pressedAtMs_ = rawPressed_ ? rawChangedAtMs_ : 0;
}

void FactoryReset::loop() {
    const unsigned long now = millis();
    const bool pressed = digitalRead(resetPin_) == LOW;
    if (pressed != rawPressed_) { rawPressed_ = pressed; rawChangedAtMs_ = now; }
    if (rawPressed_ != stablePressed_ && now - rawChangedAtMs_ >= DEBOUNCE_MS) {
        stablePressed_ = rawPressed_;
        if (stablePressed_) { pressedAtMs_ = now; resetTriggered_ = false; Serial.println("[RESET] Button pressed"); }
        else { pressedAtMs_ = 0; resetTriggered_ = false; Serial.println("[RESET] Button released"); }
    }
    if (stablePressed_ && !resetTriggered_ && pressedAtMs_ != 0 && now - pressedAtMs_ >= HOLD_TIME_MS) {
        resetTriggered_ = true;
        performReset();
    }
}

void FactoryReset::performReset() {
    Serial.println("[RESET] Factory reset triggered");
    flashLED(3, 200);
    if (settings_) { settings_->resetToDefault(); settings_->setWiFiConfigured(false); }
    Serial.println("[RESET] Restarting");
    delay(500);
    ESP.restart();
}

void FactoryReset::flashLED(uint8_t times, unsigned long durationMs) {
    for (uint8_t i = 0; i < times; ++i) {
        digitalWrite(ledPin_, HIGH); delay(durationMs);
        digitalWrite(ledPin_, LOW); delay(durationMs);
    }
}
