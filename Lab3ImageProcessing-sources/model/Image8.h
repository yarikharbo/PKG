#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace imgproc {

struct Image8 {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgb;

    Image8() = default;
    Image8(int w, int h);

    int w() const { return width; }
    int h() const { return height; }
    bool empty() const { return width <= 0 || height <= 0; }
    size_t pixelCount() const { return static_cast<size_t>(width) * height; }

    uint8_t* row(int y) { return rgb.data() + static_cast<size_t>(y) * width * 3; }
    const uint8_t* row(int y) const { return rgb.data() + static_cast<size_t>(y) * width * 3; }

    static uint8_t luminance(uint8_t r, uint8_t g, uint8_t b) {
        return static_cast<uint8_t>((299 * r + 587 * g + 114 * b) / 1000);
    }
    uint8_t grayAt(int x, int y) const;

    bool loadPPM(const std::string& path, std::string* err = nullptr);
    bool savePPM(const std::string& path, std::string* err = nullptr) const;
};

}
