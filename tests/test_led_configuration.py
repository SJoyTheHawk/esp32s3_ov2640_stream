"""Host checks: python3 -m unittest discover -s tests -v.

Compiles the real LED controller with a recording NeoPixel/FreeRTOS substitute.
Hardware timing and electrical behavior still require testing on the board.
"""
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class LedConfigurationTests(unittest.TestCase):
    def test_controller_and_persistence(self):
        compiler = shutil.which('clang++') or shutil.which('g++')
        if not compiler:
            self.skipTest('A C++ compiler is required')
        with tempfile.TemporaryDirectory() as directory:
            temp = pathlib.Path(directory)
            (temp / 'freertos').mkdir()
            (temp / 'Arduino.h').write_text('#pragma once\n#include <stdint.h>\ninline void delay(unsigned long) {}\n')
            (temp / 'freertos/FreeRTOS.h').write_text('#pragma once\n#define pdTRUE 1\n#define pdMS_TO_TICKS(ms) (ms)\n')
            (temp / 'freertos/semphr.h').write_text('''#pragma once
using SemaphoreHandle_t = void*;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return reinterpret_cast<void*>(1); }
inline int xSemaphoreTake(SemaphoreHandle_t, unsigned long) { return 1; }
inline void xSemaphoreGive(SemaphoreHandle_t) {}
inline void vSemaphoreDelete(SemaphoreHandle_t) {}
''')
            (temp / 'Adafruit_NeoPixel.h').write_text('''#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>
#define NEO_GRB 0
#define NEO_KHZ800 0
class Adafruit_NeoPixel {
    std::vector<uint32_t> pixels;
public:
    static std::vector<std::vector<uint32_t>> frames;
    static bool failAllocation;
    Adafruit_NeoPixel(uint16_t count, int, int): pixels(count) {}
    uint8_t* getPixels() { return failAllocation ? nullptr : reinterpret_cast<uint8_t*>(pixels.data()); }
    void setPin(uint8_t) {}
    bool begin() { return true; }
    void clear() { std::fill(pixels.begin(), pixels.end(), 0); }
    void show() { frames.push_back(pixels); }
    void setBrightness(uint8_t) {}
    uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return (uint32_t(r)<<16)|(uint32_t(g)<<8)|b; }
    void setPixelColor(uint16_t index, uint32_t color) { assert(index < pixels.size()); pixels[index] = color; }
};
''')
            (temp / 'test.cpp').write_text(r'''
#include "ws2812b_controller.h"
#include <cassert>
#include <iostream>
std::vector<std::vector<uint32_t>> Adafruit_NeoPixel::frames;
bool Adafruit_NeoPixel::failAllocation = false;
std::vector<int> lit() {
    std::vector<int> result;
    const auto& frame = Adafruit_NeoPixel::frames.back();
    for (size_t i=0; i<frame.size(); ++i) if (frame[i]) result.push_back(i);
    return result;
}
int main() {
    assert(LedConfig::DEFAULT_MAX_COUNT == 100);
    Ws2812bController c;
    assert(c.begin(5,10,21,true,255,20,10,10));
    assert((lit() == std::vector<int>{0,2,4,6,8}));
    assert(c.state().count == 5 && c.state().maxCount == 10);
    assert(c.reinitialize(3,10,21));
    assert((lit() == std::vector<int>{0,3,6}));
    assert(c.reinitialize(10,10,21));
    assert((lit() == std::vector<int>{0,1,2,3,4,5,6,7,8,9}));
    const auto beforeShrink = Adafruit_NeoPixel::frames.size();
    assert(c.reinitialize(1,4,21));
    const auto& cleared = Adafruit_NeoPixel::frames[beforeShrink];
    assert(cleared.size() == 10);
    for (auto pixel: cleared) assert(pixel == 0);
    assert(Adafruit_NeoPixel::frames.back().size() == 4);
    assert((lit() == std::vector<int>{0}));
    assert(c.apply(false,255,20,10,10));
    assert(lit().empty());
    assert(c.state().effectiveBrightnessPercent == 0);
    assert(c.apply(true,255,20,10,10));
    assert(c.flash(2,1,0,255,0));
    assert((lit() == std::vector<int>{0}));
    assert(c.state().red == 255 && c.state().maxCount == 4);
    for (auto invalid: {0,5}) assert(!c.reinitialize(invalid,4,21));
    assert(!c.reinitialize(1,0,21));
    assert(!c.reinitialize(1,1025,21));
    assert(!c.reinitialize(1,4,48));
    assert(c.state().count == 1 && c.state().maxCount == 4);
    Adafruit_NeoPixel::failAllocation = true;
    assert(!c.reinitialize(2,10,21));
    assert(c.state().maxCount == 4 && c.state().initialized);
    Adafruit_NeoPixel::failAllocation = false;
    assert(c.reinitialize(51,102,21));
    const auto pixels102 = lit();
    assert(pixels102.size() == 51 && pixels102.front() == 0 && pixels102.back() == 100);
    for (int i=0; i<51; ++i) assert(pixels102[i] == 2*i);
    // Check exact count, ordering, and gap balance across many uneven ratios.
    for (uint16_t maximum: {1,2,7,10,100,102,257,1024}) {
        for (uint16_t active=1; active<=maximum; ++active) {
            Adafruit_NeoPixel::frames.clear();
            assert(c.reinitialize(active,maximum,21));
            const auto selected = lit();
            assert(selected.size() == active && selected[0] == 0);
            int minGap=maximum, maxGap=0;
            for (size_t i=0; i<selected.size(); ++i) {
                int next = i+1<selected.size() ? selected[i+1] : maximum;
                int gap = next-selected[i];
                assert(gap>=1);
                minGap=std::min(minGap,gap); maxGap=std::max(maxGap,gap);
            }
            assert(maxGap-minGap<=1);
        }
    }
    Ws2812bController indicator;
    assert(indicator.begin(1,1,48,false,0,0,0,0));
    assert(Adafruit_NeoPixel::frames.back().size()==1);
    assert(!indicator.reinitialize(2,2,48));
    for (uint16_t maximum: {100,102,1024}) {
        for (uint16_t active=1; active<=maximum; ++active) {
            auto brightness=Ws2812bController::effectiveBrightness(active,maximum,100);
            const uint32_t estimate=maximum+(uint32_t(active)*60*brightness)/100;
            assert(estimate<=Ws2812bController::POWER_BUDGET_MA);
        }
    }
    assert(Ws2812bController::effectiveBrightness(0,100,100)==0);
    assert(Ws2812bController::effectiveBrightness(101,100,100)==0);
    assert(Ws2812bController::effectiveBrightness(50,100,10)==10);
    uint8_t blob[LedConfig::BLOB_SIZE];
    LedConfig::Values restored = {};
    for (uint16_t maximum: {1,100,102,256,1024}) {
        LedConfig::Values values = {true,11,22,33,44,maximum,maximum,21};
        LedConfig::encode(values,blob);
        assert(LedConfig::decode(blob,sizeof(blob),restored));
        assert(restored.enabled && restored.red==11 && restored.green==22 && restored.blue==33);
        assert(restored.brightness==44 && restored.pin==21);
        assert(restored.active==maximum && restored.maximum==maximum);
    }
    uint8_t oldBlob[] = {1,1,255,128,64,10,45,21};
    assert(LedConfig::decode(oldBlob,sizeof(oldBlob),restored));
    assert(restored.active==45 && restored.maximum==100);
    oldBlob[6]=102;
    assert(LedConfig::decode(oldBlob,sizeof(oldBlob),restored));
    assert(restored.active==102 && restored.maximum==102);
    oldBlob[6]=103;
    assert(!LedConfig::decode(oldBlob,sizeof(oldBlob),restored));
    assert(!LedConfig::decode(blob,10,restored));
    blob[0]=3;
    assert(!LedConfig::decode(blob,sizeof(blob),restored));
    LedConfig::Values invalid[] = {
        {true,0,0,0,10,11,10,21}, {true,0,0,0,10,0,10,21},
        {true,0,0,0,10,1,0,21}, {true,0,0,0,10,1,1025,21},
        {true,0,0,0,101,1,10,21}, {true,0,0,0,10,1,10,48}
    };
    for (const auto& v: invalid) {
        LedConfig::encode(v,blob);
        assert(!LedConfig::decode(blob,sizeof(blob),restored));
    }
    std::cout << "Controller output, spacing, clearing, limits, and persistence checks passed\n";
}
''')
            executable = temp / 'led_tests'
            subprocess.run([compiler, '-std=c++11', '-Wall', '-Wextra', '-Werror',
                            '-I' + str(temp), '-I' + str(ROOT / 'v3_ino_2'),
                            str(temp / 'test.cpp'), str(ROOT / 'v3_ino_2/ws2812b_controller.cpp'),
                            '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

    def test_embedded_javascript_syntax(self):
        node = shutil.which('node')
        if not node:
            self.skipTest('Node is required')
        source = (ROOT / 'v3_ino_2/html_pages.h').read_text()
        scripts = re.findall(r'<script>(.*?)</script>', source, re.S)
        self.assertTrue(scripts)
        for script in scripts:
            subprocess.run([node, '--check'], input=script, text=True, check=True)


if __name__ == '__main__':
    unittest.main()
