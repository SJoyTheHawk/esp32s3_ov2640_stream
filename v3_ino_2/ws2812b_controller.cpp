#include "ws2812b_controller.h"
#include <new>

Ws2812bController::Ws2812bController()
    : strip_(nullptr), mutex_(nullptr) {}

Ws2812bController::~Ws2812bController() {
    if (strip_) {
        strip_->clear();
        strip_->show();
        delete strip_;
    }
    if (mutex_) vSemaphoreDelete(mutex_);
}

bool Ws2812bController::isValidCount(uint16_t count, uint16_t maxCount) {
    return LedConfig::validCounts(count, maxCount);
}

bool Ws2812bController::isValidDataPin(uint8_t dataPin) {
    // GPIO 21 is the initial board-approved DIN pin. Extend this list only
    // after confirming the board schematic and reserved camera pins.
    return dataPin == 21;
}

bool Ws2812bController::isValidIndicatorPin(uint8_t dataPin) {
    // The board's single onboard WS2812B uses GPIO 48 as its DIN pin.
    return dataPin == 48;
}

uint8_t Ws2812bController::effectiveBrightness(uint16_t count, uint16_t maxCount, uint8_t requestedPercent) {
    if (requestedPercent > 100 || !isValidCount(count, maxCount)) return 0;
    const uint32_t available = POWER_BUDGET_MA > maxCount ? POWER_BUDGET_MA - maxCount : 0;
    const uint32_t maxPercent = (available * 100U) /
                                (static_cast<uint32_t>(count) * CURRENT_PER_LED_MA);
    const uint8_t safeMaximum = static_cast<uint8_t>(maxPercent > 100 ? 100 : maxPercent);
    return requestedPercent < safeMaximum ? requestedPercent : safeMaximum;
}

bool Ws2812bController::reinitializeLocked(uint16_t count, uint16_t maxCount, uint8_t dataPin) {
    const bool validPin = isValidDataPin(dataPin) ||
                          (count == 1 && maxCount == 1 && isValidIndicatorPin(dataPin));
    if (!isValidCount(count, maxCount) || !validPin) return false;
    // Transmit the complete configured strip, clearing inactive pixels. Clear
    // the old buffer before shrinking so previously lit tail pixels turn off.
    // Construct unbound to avoid touching the live pin if allocation fails.
    Adafruit_NeoPixel* next = new (std::nothrow) Adafruit_NeoPixel(maxCount, -1, NEO_GRB + NEO_KHZ800);
    if (!next) return false;
    if (!next->getPixels()) { delete next; return false; }
    if (strip_) {
        strip_->clear();
        strip_->show();
        delete strip_;
    }
    strip_ = next;
    strip_->setPin(dataPin);
    if (!strip_->begin()) {
        delete strip_; strip_ = nullptr; state_.initialized = false;
        return false;
    }
    state_.initialized = true;
    state_.count = count;
    state_.maxCount = maxCount;
    state_.dataPin = dataPin;
    return applyLocked();
}

bool Ws2812bController::begin(uint16_t count, uint16_t maxCount, uint8_t dataPin, bool enabled,
                              uint8_t red, uint8_t green, uint8_t blue,
                              uint8_t brightnessPercent) {
    if (!mutex_) mutex_ = xSemaphoreCreateMutex();
    const bool validPin = isValidDataPin(dataPin) ||
                          (count == 1 && maxCount == 1 && isValidIndicatorPin(dataPin));
    if (!isValidCount(count, maxCount) || !validPin || !mutex_ || brightnessPercent > 100 ||
        xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE) return false;
    state_.enabled = enabled;
    state_.red = red;
    state_.green = green;
    state_.blue = blue;
    state_.brightnessPercent = brightnessPercent;
    const bool ok = reinitializeLocked(count, maxCount, dataPin);
    xSemaphoreGive(mutex_);
    return ok;
}

bool Ws2812bController::reinitialize(uint16_t count, uint16_t maxCount, uint8_t dataPin) {
    if (!mutex_ || xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE) return false;
    const bool ok = reinitializeLocked(count, maxCount, dataPin);
    xSemaphoreGive(mutex_);
    return ok;
}

bool Ws2812bController::apply(bool enabled, uint8_t red, uint8_t green,
                              uint8_t blue, uint8_t brightnessPercent) {
    if (!mutex_ || brightnessPercent > 100 || xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE) return false;
    if (!state_.initialized) { xSemaphoreGive(mutex_); return false; }
    state_.enabled = enabled;
    state_.red = red;
    state_.green = green;
    state_.blue = blue;
    state_.brightnessPercent = brightnessPercent;
    const bool ok = state_.initialized && applyLocked();
    xSemaphoreGive(mutex_);
    return ok;
}

bool Ws2812bController::flash(uint8_t times, unsigned long durationMs,
                              uint8_t red, uint8_t green, uint8_t blue) {
    if (times == 0) return true;
    const State previous = state();
    if (!previous.initialized) return false;

    for (uint8_t i = 0; i < times; ++i) {
        if (!apply(true, red, green, blue, 100)) return false;
        delay(durationMs);
        if (!apply(false, 0, 0, 0, 0)) return false;
        delay(durationMs);
    }
    return apply(previous.enabled, previous.red, previous.green, previous.blue,
                 previous.brightnessPercent);
}

bool Ws2812bController::applyLocked() {
    if (!strip_ || !state_.initialized) return false;
    state_.effectiveBrightnessPercent = state_.enabled ? effectiveBrightness(state_.count, state_.maxCount, state_.brightnessPercent) : 0;
    const uint8_t hardwareBrightness = static_cast<uint8_t>(
        (static_cast<uint16_t>(state_.effectiveBrightnessPercent) * 255U) / 100U);
    strip_->setBrightness(hardwareBrightness);
    const uint32_t color = state_.enabled ? strip_->Color(state_.red, state_.green, state_.blue) : 0;
    strip_->clear();
    // floor(i * maximum / active) distributes exactly count distinct pixels.
    // For maximum=10 and count=5, this selects 0, 2, 4, 6, 8.
    for (uint16_t i = 0; i < state_.count; ++i) {
        const uint16_t index = static_cast<uint16_t>(
            (static_cast<uint32_t>(i) * state_.maxCount) / state_.count);
        strip_->setPixelColor(index, color);
    }
    strip_->show();
    return true;
}

Ws2812bController::State Ws2812bController::state() const {
    State copy;
    if (!mutex_ || xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE) return copy;
    copy = state_;
    xSemaphoreGive(mutex_);
    return copy;
}
