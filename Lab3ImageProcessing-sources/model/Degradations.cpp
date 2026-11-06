#include "Degradations.h"

#include <algorithm>
#include <cmath>

namespace imgproc {

namespace {

unsigned xorshift32(unsigned& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

double uniform01(unsigned& state) {
    return (xorshift32(state) & 0xFFFFFF) / 16777215.0;
}

double gaussian(unsigned& state) {
    double u1 = uniform01(state);
    while (u1 <= 1e-9) u1 = uniform01(state);
    const double u2 = uniform01(state);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(6.283185307179586 * u2);
}

}

Image8 addGaussianNoise(const Image8& img, double sigma, unsigned seed) {
    unsigned state = seed ? seed : 0x9E3779B9u;
    Image8 out(img.w(), img.h());
    for (size_t i = 0; i < img.rgb.size(); ++i) {
        const double n = img.rgb[i] + sigma * gaussian(state);
        out.rgb[i] = static_cast<uint8_t>(std::clamp(std::lround(n), 0L, 255L));
    }
    return out;
}

Image8 addSaltPepper(const Image8& img, double probability, unsigned seed) {
    unsigned state = seed ? seed : 0xDEADBEEFu;
    Image8 out = img;
    for (size_t i = 0; i < out.rgb.size(); i += 3) {
        if (uniform01(state) < probability) {
            const uint8_t v = uniform01(state) < 0.5 ? 0 : 255;
            out.rgb[i] = out.rgb[i + 1] = out.rgb[i + 2] = v;
        }
    }
    return out;
}

Image8 squeezeContrast(const Image8& img, int lowLevel, int highLevel) {
    lowLevel = std::clamp(lowLevel, 0, 254);
    highLevel = std::clamp(highLevel, lowLevel + 1, 255);
    const double scale = (highLevel - lowLevel) / 255.0;
    Image8 out(img.w(), img.h());
    for (size_t i = 0; i < img.rgb.size(); ++i) {
        out.rgb[i] = static_cast<uint8_t>(std::lround(lowLevel + img.rgb[i] * scale));
    }
    return out;
}

}
