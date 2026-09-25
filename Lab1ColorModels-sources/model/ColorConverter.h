#ifndef COLORCONVERTER_H
#define COLORCONVERTER_H

// Model layer: pure color-space math (variant 3: RGB <-> XYZ <-> Lab).
// This unit depends on nothing but the C++ standard library:
// no Qt, no GUI, no I/O. It follows the formulas from the method sheet
// "Формулы преобразования цветовых моделей" (sRGB, D65 white point).

#include <array>

namespace colormodel {

struct Rgb {
    int r = 0;
    int g = 0;
    int b = 0;
};

struct Xyz {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Lab {
    double l = 0.0;
    double a = 0.0;
    double b = 0.0;
};

// Result of XYZ -> RGB with per-channel out-of-gamut report.
struct RgbFromXyz {
    Rgb value;                                   // clamped to 0..255
    bool clipped = false;                        // true if any channel was out of gamut
    std::array<bool, 3> clippedChannels{};       // R, G, B flags
};

// D65 white point and CIE constants from the method sheet.
constexpr double kWhiteX = 95.047;
constexpr double kWhiteY = 100.0;
constexpr double kWhiteZ = 108.883;
constexpr double kCieEpsilon = 0.008856;
constexpr double kCieK = 7.787;

// sRGB transfer (gamma) functions, c8 in 0..255 / v in 0..1.
double SrgbToLinear(double c8);
double LinearToSrgb(double v);

// RGB -> XYZ. X, Y, Z are in the 0..100 scale of the method sheet.
Xyz RgbToXyz(const Rgb& rgb);

// XYZ -> RGB. Values outside the sRGB gamut are clamped and reported.
RgbFromXyz XyzToRgbDetailed(const Xyz& xyz);

// XYZ <-> Lab (D65, per the method sheet).
Lab XyzToLab(const Xyz& xyz);
Xyz LabToXyz(const Lab& lab);

// Convenience: true if the XYZ color lies inside the sRGB gamut.
bool XyzInSRgbGamut(const Xyz& xyz);

}  // namespace colormodel

#endif  // COLORCONVERTER_H
