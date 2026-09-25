#include "ColorConverter.h"

#include <algorithm>
#include <cmath>

namespace colormodel {

namespace {

// The method sheet's XYZ -> RGB matrix is rounded to 4 decimals and is not an
// exact inverse of its 6-decimal forward matrix. Roundtrip noise reaches
// ~2.9e-4 in the linear domain (max |M_inv*M - I| = 2.83e-4). This tolerance
// separates that rounding noise from genuine out-of-gamut colors; 5e-4 in the
// linear domain corresponds to <= ~0.03/255 in sRGB, i.e. invisible.
constexpr double kGamutTolerance = 5e-4;

double LabF(double t) {
    return t > kCieEpsilon ? std::cbrt(t) : kCieK * t + 16.0 / 116.0;
}

// Symmetric inverse of LabF: for f^3 <= eps the linear branch is
// (f - 16/116) / 7.787, which equals L / (116 * 7.787) = L / 903.3.
double LabFInverse(double f) {
    const double c = f * f * f;
    return c > kCieEpsilon ? c : (f - 16.0 / 116.0) / kCieK;
}

}  // namespace

double SrgbToLinear(double c8) {
    const double x = c8 / 255.0;
    return x > 0.04045 ? std::pow((x + 0.055) / 1.055, 2.4) : x / 12.92;
}

double LinearToSrgb(double v) {
    const double s = v > 0.0031308 ? 1.055 * std::pow(v, 1.0 / 2.4) - 0.055 : 12.92 * v;
    return s * 255.0;
}

Xyz RgbToXyz(const Rgb& rgb) {
    const double r = SrgbToLinear(rgb.r);
    const double g = SrgbToLinear(rgb.g);
    const double b = SrgbToLinear(rgb.b);
    Xyz out;
    out.x = (0.412453 * r + 0.357580 * g + 0.180423 * b) * 100.0;
    out.y = (0.212671 * r + 0.715160 * g + 0.072169 * b) * 100.0;
    out.z = (0.019334 * r + 0.119193 * g + 0.950227 * b) * 100.0;
    return out;
}

RgbFromXyz XyzToRgbDetailed(const Xyz& xyz) {
    const double xn = xyz.x / 100.0;
    const double yn = xyz.y / 100.0;
    const double zn = xyz.z / 100.0;

    const std::array<double, 3> lin{
        3.2406 * xn - 1.5372 * yn - 0.4986 * zn,
        -0.9689 * xn + 1.8758 * yn + 0.0415 * zn,
        0.0557 * xn - 0.2040 * yn + 1.0570 * zn};

    RgbFromXyz out;
    for (int i = 0; i < 3; ++i) {
        out.clippedChannels[i] = lin[i] < -kGamutTolerance || lin[i] > 1.0 + kGamutTolerance;
        out.clipped = out.clipped || out.clippedChannels[i];
        const double v = std::clamp(lin[i], 0.0, 1.0);
        const int c = static_cast<int>(std::lround(LinearToSrgb(v)));
        switch (i) {
            case 0: out.value.r = std::clamp(c, 0, 255); break;
            case 1: out.value.g = std::clamp(c, 0, 255); break;
            default: out.value.b = std::clamp(c, 0, 255); break;
        }
    }
    return out;
}

Lab XyzToLab(const Xyz& xyz) {
    const double fx = LabF(xyz.x / kWhiteX);
    const double fy = LabF(xyz.y / kWhiteY);
    const double fz = LabF(xyz.z / kWhiteZ);
    Lab out;
    out.l = 116.0 * fy - 16.0;
    out.a = 500.0 * (fx - fy);
    out.b = 200.0 * (fy - fz);
    return out;
}

Xyz LabToXyz(const Lab& lab) {
    const double fy = (lab.l + 16.0) / 116.0;
    const double fx = fy + lab.a / 500.0;
    const double fz = fy - lab.b / 200.0;
    Xyz out;
    out.x = LabFInverse(fx) * kWhiteX;
    out.y = LabFInverse(fy) * kWhiteY;
    out.z = LabFInverse(fz) * kWhiteZ;
    return out;
}

bool XyzInSRgbGamut(const Xyz& xyz) {
    return !XyzToRgbDetailed(xyz).clipped;
}

}  // namespace colormodel
