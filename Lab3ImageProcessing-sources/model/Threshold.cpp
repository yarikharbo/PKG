#include "Threshold.h"

#include "Contrast.h"

#include <algorithm>
#include <cmath>

namespace imgproc {

Image8 toGrayscaleBin(const Image8& img, int threshold) {
    Image8 out(img.w(), img.h());
    for (int y = 0; y < img.h(); ++y) {
        const uint8_t* src = img.row(y);
        uint8_t* dst = out.row(y);
        for (int x = 0; x < img.w(); ++x) {
            const uint8_t g = Image8::luminance(src[x * 3], src[x * 3 + 1], src[x * 3 + 2]);
            const uint8_t v = g > threshold ? 255 : 0;
            dst[x * 3] = dst[x * 3 + 1] = dst[x * 3 + 2] = v;
        }
    }
    return out;
}

int otsuThreshold(const std::vector<int>& hist) {
    long long total = 0;
    long long sumAll = 0;
    for (int i = 0; i < 256; ++i) {
        total += hist[i];
        sumAll += static_cast<long long>(i) * hist[i];
    }
    if (total == 0) return 0;
    long long wB = 0;
    long long sumB = 0;
    int bestT = 0;
    double bestVar = -1.0;
    for (int t = 0; t < 256; ++t) {
        wB += hist[t];
        if (wB == 0) continue;
        const long long wF = total - wB;
        if (wF == 0) break;
        sumB += static_cast<long long>(t) * hist[t];
        const double muB = static_cast<double>(sumB) / wB;
        const double muF = static_cast<double>(sumAll - sumB) / wF;
        const double between = static_cast<double>(wB) * wF * (muB - muF) * (muB - muF);
        if (between > bestVar) {
            bestVar = between;
            bestT = t;
        }
    }
    return bestT;
}

Image8 thresholdOtsu(const Image8& img) {
    return toGrayscaleBin(img, otsuThreshold(histogramGray(img)));
}

Image8 thresholdAdaptive(const Image8& img, int windowSize, int offsetC) {
    const int r = std::max(1, windowSize / 2);
    const int w = img.w(), h = img.h();

    std::vector<long long> sat(static_cast<size_t>(w + 1) * (h + 1), 0);
    for (int y = 0; y < h; ++y) {
        long long rowSum = 0;
        for (int x = 0; x < w; ++x) {
            rowSum += Image8::luminance(img.row(y)[x * 3], img.row(y)[x * 3 + 1], img.row(y)[x * 3 + 2]);
            sat[static_cast<size_t>(y + 1) * (w + 1) + (x + 1)] =
                sat[static_cast<size_t>(y) * (w + 1) + (x + 1)] + rowSum;
        }
    }

    Image8 out(img.w(), img.h());
    for (int y = 0; y < h; ++y) {
        uint8_t* dst = out.row(y);
        const int y0 = std::max(0, y - r);
        const int y1 = std::min(h - 1, y + r);
        for (int x = 0; x < w; ++x) {
            const int x0 = std::max(0, x - r);
            const int x1 = std::min(w - 1, x + r);
            const long long area = static_cast<long long>(x1 - x0 + 1) * (y1 - y0 + 1);
            const long long sum = sat[static_cast<size_t>(y1 + 1) * (w + 1) + (x1 + 1)] -
                                  sat[static_cast<size_t>(y0) * (w + 1) + (x1 + 1)] -
                                  sat[static_cast<size_t>(y1 + 1) * (w + 1) + x0] +
                                  sat[static_cast<size_t>(y0) * (w + 1) + x0];
            const double mean = static_cast<double>(sum) / area;
            const uint8_t g = Image8::luminance(img.row(y)[x * 3], img.row(y)[x * 3 + 1], img.row(y)[x * 3 + 2]);
            const uint8_t v = g > mean - offsetC ? 255 : 0;
            dst[x * 3] = dst[x * 3 + 1] = dst[x * 3 + 2] = v;
        }
    }
    return out;
}

}
