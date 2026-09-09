#include "weight_sensor.h"

#include <cmath>
#include <cstdlib>

static portMUX_TYPE hx711Mux = portMUX_INITIALIZER_UNLOCKED;

WeightSensor::WeightSensor(uint8_t dataPin, uint8_t clockPin, CameraSettings* settings)
    : dataPin_(dataPin), clockPin_(clockPin), settings_(settings) {}

void WeightSensor::begin() {
    pinMode(dataPin_, INPUT);
    pinMode(clockPin_, OUTPUT);
    digitalWrite(clockPin_, LOW);
    readMutex_ = xSemaphoreCreateMutex();
    portENTER_CRITICAL(&snapshotMux_);
    reading_ = WeightReading();
    portEXIT_CRITICAL(&snapshotMux_);
}

void WeightSensor::startTask() {
    xTaskCreatePinnedToCore(taskEntry, "HX711", 4096, this, 1, &taskHandle_, 1);
}

WeightReading WeightSensor::latest() const {
    WeightReading copy;
    portENTER_CRITICAL(&snapshotMux_);
    copy = reading_;
    portEXIT_CRITICAL(&snapshotMux_);
    if (copy.valid && millis() - copy.sampledAtMs > 1000UL) copy.valid = false;
    return copy;
}

void WeightSensor::taskEntry(void* arg) { static_cast<WeightSensor*>(arg)->taskLoop(); }

void WeightSensor::taskLoop() {
    int32_t raw = 0;
    float previous = 0.0f;
    for (;;) {
        const bool ok = readRaw(raw);
        WeightReading next;
        next.sampledAtMs = millis();
        if (ok && settings_) {
            next.raw = raw;
            next.grams = (static_cast<float>(raw) - settings_->weightOffset) / settings_->weightScale;
            next.valid = isfinite(next.grams);
            next.stable = next.valid && fabsf(next.grams - previous) < 1.0f;
            previous = next.grams;
        }
        portENTER_CRITICAL(&snapshotMux_);
        reading_ = next;
        portEXIT_CRITICAL(&snapshotMux_);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool WeightSensor::readRaw(int32_t& value, uint32_t timeoutMs) {
    if (!readMutex_ || xSemaphoreTake(readMutex_, pdMS_TO_TICKS(timeoutMs)) != pdTRUE) return false;
    const uint32_t start = millis();
    while (digitalRead(dataPin_) != LOW) {
        if (millis() - start >= timeoutMs) { xSemaphoreGive(readMutex_); return false; }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    uint32_t result = 0;
    portENTER_CRITICAL(&hx711Mux);
    for (uint8_t i = 0; i < 24; ++i) {
        digitalWrite(clockPin_, HIGH);
        result = (result << 1) | (digitalRead(dataPin_) ? 1U : 0U);
        digitalWrite(clockPin_, LOW);
    }
    digitalWrite(clockPin_, HIGH);
    digitalWrite(clockPin_, LOW);
    portEXIT_CRITICAL(&hx711Mux);
    xSemaphoreGive(readMutex_);
    value = (result & 0x800000U) ? static_cast<int32_t>(result | 0xFF000000U)
                                 : static_cast<int32_t>(result);
    return true;
}

bool WeightSensor::readAverage(int32_t& value, uint8_t count) {
    int64_t total = 0;
    for (uint8_t i = 0; i < count; ++i) {
        int32_t sample;
        if (!readRaw(sample, 500)) return false;
        total += sample;
    }
    value = static_cast<int32_t>(total / count);
    return true;
}

bool WeightSensor::tare(int32_t& offset) {
    if (!settings_ || !readAverage(offset, 16)) return false;
    return settings_->writeWeightOffset(offset);
}

bool WeightSensor::calibrate(float knownGrams, float& scale) {
    if (!settings_ || !isfinite(knownGrams) || knownGrams <= 0.0f) return false;
    int32_t raw;
    if (!readAverage(raw, 4)) return false;
    const int32_t delta = raw - settings_->weightOffset;
    if (labs(static_cast<long>(delta)) < 10) return false;
    scale = static_cast<float>(delta) / knownGrams;
    return isfinite(scale) && scale != 0.0f && settings_->writeWeightScale(scale);
}
