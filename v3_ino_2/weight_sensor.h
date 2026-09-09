#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include <Arduino.h>
#include "camera_settings.h"

struct WeightReading {
    float grams = 0.0f;
    int32_t raw = 0;
    uint32_t sampledAtMs = 0;
    bool valid = false;
    bool stable = false;
};

class WeightSensor {
public:
    WeightSensor(uint8_t dataPin, uint8_t clockPin, CameraSettings* settings);
    void begin();
    void startTask();
    WeightReading latest() const;
    bool tare(int32_t& offset);
    bool calibrate(float knownGrams, float& scale);

private:
    static void taskEntry(void* arg);
    void taskLoop();
    bool readRaw(int32_t& value, uint32_t timeoutMs = 250);
    bool readAverage(int32_t& value, uint8_t count);

    uint8_t dataPin_;
    uint8_t clockPin_;
    CameraSettings* settings_;
    WeightReading reading_;
    TaskHandle_t taskHandle_ = nullptr;
    mutable portMUX_TYPE snapshotMux_ = portMUX_INITIALIZER_UNLOCKED;
    SemaphoreHandle_t readMutex_ = nullptr;
};

#endif
