#include "Contrast.h"

#include <algorithm>
#include <cmath>

namespace imgproc {

std::vector<int> histogramGray(const Image8& img) {
    std::vector<int> hist(256, 0);
    for (int y = 0; y < img.h(); ++y) {
        const uint8_t* row = img.row(y);
        for (int x = 0; x < img.w(); ++x) {
            ++hist[Image8::luminance(row[x * 3], row[x * 3 + 1], row[x * 3 + 2])];
        }
    }
    return hist;
}

std::array<std::vector<int>, 3> histogramChannels(const Image8& img) {
    std::array<std::vector<int>, 3> hist = {std::vector<int>(256, 0), std::vector<int>(256, 0),
                                            std::vector<int>(256, 0)};
    for (int y = 0; y < img.h(); ++y) {
        const uint8_t* row = img.row(y);
        for (int x = 0; x < img.w(); ++x) {
            ++hist[0][row[x * 3]];
            ++hist[1][row[x * 3 + 1]];
            ++hist[2][row[x * 3 + 2]];
        }
    }
    return hist;
}

std::vector<uint8_t> linearContrastLUT(int blackPoint, int whitePoint) {
    blackPoint = std::clamp(blackPoint, 0, 255);
    whitePoint = std::clamp(whitePoint, 0, 255);
    if (whitePoint < blackPoint) std::swap(blackPoint, whitePoint);
    std::vector<uint8_t> lut(256);
    if (whitePoint == blackPoint) {
        for (int i = 0; i < 256; ++i) lut[i] = static_cast<uint8_t>(i < blackPoint ? 0 : 255);
        return lut;
    }
    const double scale = 255.0 / (whitePoint - blackPoint);
    for (int i = 0; i < 256; ++i) {
        const double v = (i - blackPoint) * scale;
        lut[i] = static_cast<uint8_t>(std::clamp(std::lround(v), 0L, 255L));
    }
    return lut;
}

std::vector<uint8_t> equalizeLUT(const std::vector<int>& hist) {
    long long total = 0;
    for (int v : hist) total += v;
    std::vector<uint8_t> lut(256, 0);
    if (total == 0) {
        for (int i = 0; i < 256; ++i) lut[i] = static_cast<uint8_t>(i);
        return lut;
    }
    long long cdf = 0;
    for (int i = 0; i < 256; ++i) {
        cdf += hist[i];
        lut[i] = static_cast<uint8_t>(std::lround(255.0 * cdf / total));
    }
    return lut;
}

Image8 applyLUT(const Image8& img, const std::vector<uint8_t>& lut) {
    Image8 out(img.w(), img.h());
    for (size_t i = 0; i < img.rgb.size(); ++i) out.rgb[i] = lut[img.rgb[i]];
    return out;
}

Image8 linearContrast(const Image8& img, int blackPoint, int whitePoint) {
    return applyLUT(img, linearContrastLUT(blackPoint, whitePoint));
}

Image8 toGrayscale(const Image8& img) {
    Image8 out(img.w(), img.h());
    for (int y = 0; y < img.h(); ++y) {
        const uint8_t* src = img.row(y);
        uint8_t* dst = out.row(y);
        for (int x = 0; x < img.w(); ++x) {
            const uint8_t g = Image8::luminance(src[x * 3], src[x * 3 + 1], src[x * 3 + 2]);
            dst[x * 3] = dst[x * 3 + 1] = dst[x * 3 + 2] = g;
        }
    }
    return out;
}

namespace {

void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, int& h255, int& s255, int& v255) {
    const int maxc = std::max({r, g, b});
    const int minc = std::min({r, g, b});
    const int d = maxc - minc;
    v255 = maxc;
    s255 = maxc == 0 ? 0 : static_cast<int>(std::lround(255.0 * d / maxc));
    if (d == 0) {
        h255 = 0;
        return;
    }
    double hdeg;
    if (maxc == r) hdeg = 60.0 * (g - b) / d;
    else if (maxc == g) hdeg = 120.0 + 60.0 * (b - r) / d;
    else hdeg = 240.0 + 60.0 * (r - g) / d;
    if (hdeg < 0) hdeg += 360.0;
    h255 = static_cast<int>(std::lround(hdeg * 255.0 / 360.0));
}

void hsvToRgb(int h255, int s255, int v255, uint8_t& r, uint8_t& g, uint8_t& b) {
    const double hf = h255 * 360.0 / 255.0 / 60.0;
    const double s = s255 / 255.0;
    const double v = v255 / 255.0;
    const int i = static_cast<int>(hf) % 6;
    const double f = hf - i;
    const double p = v * (1.0 - s);
    const double q = v * (1.0 - s * f);
    const double t = v * (1.0 - s * (1.0 - f));
    double rr = 0, gg = 0, bb = 0;
    switch (i) {
    case 0: rr = v; gg = t; bb = p; break;
    case 1: rr = q; gg = v; bb = p; break;
    case 2: rr = p; gg = v; bb = t; break;
    case 3: rr = p; gg = q; bb = v; break;
    case 4: rr = t; gg = p; bb = v; break;
    default: rr = v; gg = p; bb = q; break;
    }
    r = static_cast<uint8_t>(std::lround(rr * 255.0));
    g = static_cast<uint8_t>(std::lround(gg * 255.0));
    b = static_cast<uint8_t>(std::lround(bb * 255.0));
}

}

Image8 equalizeGlobal(const Image8& img, EqualizeMode mode) {
    if (mode == EqualizeMode::Gray) {
        return applyLUT(toGrayscale(img), equalizeLUT(histogramGray(img)));
    }
    if (mode == EqualizeMode::PerChannelRGB) {
        Image8 out = img;
        auto hists = histogramChannels(img);
        for (int c = 0; c < 3; ++c) {
            const std::vector<uint8_t> lut = equalizeLUT(hists[c]);
            for (size_t i = c; i < out.rgb.size(); i += 3) out.rgb[i] = lut[out.rgb[i]];
        }
        return out;
    }
    Image8 out(img.w(), img.h());
    std::vector<int> vh(256, 0);
    std::vector<int> hh(img.pixelCount()), ss(img.pixelCount()), vv(img.pixelCount());
    for (size_t i = 0; i < img.pixelCount(); ++i) {
        int h = 0, s = 0, v = 0;
        rgbToHsv(img.rgb[i * 3], img.rgb[i * 3 + 1], img.rgb[i * 3 + 2], h, s, v);
        hh[i] = h;
        ss[i] = s;
        vv[i] = v;
        ++vh[v];
    }
    const std::vector<uint8_t> lut = equalizeLUT(vh);
    for (size_t i = 0; i < img.pixelCount(); ++i) {
        uint8_t r = 0, g = 0, b = 0;
        hsvToRgb(hh[i], ss[i], lut[vv[i]], r, g, b);
        out.rgb[i * 3] = r;
        out.rgb[i * 3 + 1] = g;
        out.rgb[i * 3 + 2] = b;
    }
    return out;
}

}
