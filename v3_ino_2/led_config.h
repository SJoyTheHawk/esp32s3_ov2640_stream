#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include <stddef.h>
#include <stdint.h>

namespace LedConfig {
constexpr uint16_t DEFAULT_MAX_COUNT = 100;
// Resource guard for the pixel buffer; the configured strip length is variable.
constexpr uint16_t MAX_SUPPORTED_COUNT = 1024;
constexpr size_t BLOB_SIZE = 11;
constexpr size_t LEGACY_BLOB_SIZE = 8;

inline bool validCounts(uint16_t active, uint16_t maximum) {
    return maximum >= 1 && maximum <= MAX_SUPPORTED_COUNT &&
           active >= 1 && active <= maximum;
}

struct Values {
    bool enabled;
    uint8_t red, green, blue, brightness;
    uint16_t active, maximum;
    uint8_t pin;
};

inline void encode(const Values& v, uint8_t* blob) {
    // v2: version, enabled, RGB, brightness, active LE16, maximum LE16, pin.
    blob[0] = 2; blob[1] = v.enabled;
    blob[2] = v.red; blob[3] = v.green; blob[4] = v.blue;
    blob[5] = v.brightness;
    blob[6] = static_cast<uint8_t>(v.active);
    blob[7] = static_cast<uint8_t>(v.active >> 8);
    blob[8] = static_cast<uint8_t>(v.maximum);
    blob[9] = static_cast<uint8_t>(v.maximum >> 8);
    blob[10] = v.pin;
}

inline bool decode(const uint8_t* blob, size_t length, Values& output) {
    if (length != BLOB_SIZE && length != LEGACY_BLOB_SIZE) return false;
    Values v = {};
    if (length == BLOB_SIZE && blob[0] == 2) {
        v.active = static_cast<uint16_t>(blob[6]) | (static_cast<uint16_t>(blob[7]) << 8);
        v.maximum = static_cast<uint16_t>(blob[8]) | (static_cast<uint16_t>(blob[9]) << 8);
        v.pin = blob[10];
    } else if (length == LEGACY_BLOB_SIZE && blob[0] == 1) {
        if (blob[6] < 1 || blob[6] > 102) return false;
        v.active = blob[6];
        // Preserve valid old active counts, including installations with 102 LEDs.
        v.maximum = v.active > DEFAULT_MAX_COUNT ? v.active : DEFAULT_MAX_COUNT;
        v.pin = blob[7];
    } else {
        return false;
    }
    if (blob[1] > 1 || blob[5] > 100 || v.pin != 21 ||
        !validCounts(v.active, v.maximum)) return false;
    v.enabled = blob[1]; v.red = blob[2]; v.green = blob[3]; v.blue = blob[4];
    v.brightness = blob[5];
    output = v;
    return true;
}
} // namespace LedConfig

#endif
