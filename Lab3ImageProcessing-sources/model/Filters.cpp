#include "Filters.h"

#include <algorithm>
#include <cmath>

namespace imgproc {

namespace {

int edgeIndex(int v, int n, EdgeMode em) {
    if (n == 1) return 0;
    if (v < 0 || v >= n) {
        if (em == EdgeMode::Clamp) {
            v = std::clamp(v, 0, n - 1);
        } else {
            while (v < 0) v = -1 - v;
            while (v >= n) v = 2 * n - 2 - v;
            v = std::clamp(v, 0, n - 1);
        }
    }
    return v;
}

uint8_t clampByte(double v) {
    return static_cast<uint8_t>(std::clamp(std::lround(v), 0L, 255L));
}

}

Kernel2D makeLaplacian(int connectivity) {
    Kernel2D k;
    k.size = 3;
    k.data.assign(9, -1.0);
    if (connectivity == 4) {
        k.data[1] = -1.0; k.data[3] = -1.0; k.data[4] = 4.0; k.data[5] = -1.0; k.data[7] = -1.0;
        k.data[0] = k.data[2] = k.data[6] = k.data[8] = 0.0;
    } else {
        k.data[4] = 8.0;
    }
    return k;
}

Kernel1D makeGaussian1D(int radius, double sigma) {
    if (radius < 1) return Kernel1D{1.0};
    const int size = 2 * radius + 1;
    Kernel1D k(static_cast<size_t>(size));
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        const double d = i - radius;
        k[static_cast<size_t>(i)] = std::exp(-(d * d) / (2.0 * sigma * sigma));
        sum += k[static_cast<size_t>(i)];
    }
    for (double& v : k) v /= sum;
    return k;
}

Kernel1D makeBox1D(int radius) {
    if (radius < 1) return Kernel1D{1.0};
    return Kernel1D(static_cast<size_t>(2 * radius + 1), 1.0 / (2 * radius + 1));
}

Image8 convolve(const Image8& img, const Kernel2D& k, EdgeMode em) {
    const int r = k.size / 2;
    Image8 out(img.w(), img.h());
    for (int y = 0; y < img.h(); ++y) {
        for (int x = 0; x < img.w(); ++x) {
            double acc[3] = {0.0, 0.0, 0.0};
            for (int ky = 0; ky < k.size; ++ky) {
                const int sy = edgeIndex(y + ky - r, img.h(), em);
                const uint8_t* srow = img.row(sy);
                for (int kx = 0; kx < k.size; ++kx) {
                    const int sx = edgeIndex(x + kx - r, img.w(), em);
                    const uint8_t* p = srow + static_cast<size_t>(sx) * 3;
                    const double w = k.data[static_cast<size_t>(ky) * k.size + kx];
                    acc[0] += w * p[0];
                    acc[1] += w * p[1];
                    acc[2] += w * p[2];
                }
            }
            uint8_t* d = out.row(y) + static_cast<size_t>(x) * 3;
            d[0] = clampByte(acc[0]);
            d[1] = clampByte(acc[1]);
            d[2] = clampByte(acc[2]);
        }
    }
    return out;
}

Image8 boxBlur(const Image8& img, int radius, EdgeMode em) {
    if (radius < 1) return img;
    const int size = 2 * radius + 1;
    Kernel2D k{size, std::vector<double>(static_cast<size_t>(size) * size, 1.0 / (size * size))};
    return convolve(img, k, em);
}

Image8 gaussianBlur(const Image8& img, int radius, EdgeMode em) {
    if (radius < 1) return img;
    const Kernel1D g = makeGaussian1D(radius, radius / 3.0);
    const int size = static_cast<int>(g.size());
    const int r = radius;

    Image8 tmp(img.w(), img.h());
    for (int y = 0; y < img.h(); ++y) {
        const uint8_t* srow = img.row(y);
        uint8_t* drow = tmp.row(y);
        for (int x = 0; x < img.w(); ++x) {
            double acc[3] = {0.0, 0.0, 0.0};
            for (int k = 0; k < size; ++k) {
                const int sx = edgeIndex(x + k - r, img.w(), em);
                const uint8_t* p = srow + static_cast<size_t>(sx) * 3;
                acc[0] += g[static_cast<size_t>(k)] * p[0];
                acc[1] += g[static_cast<size_t>(k)] * p[1];
                acc[2] += g[static_cast<size_t>(k)] * p[2];
            }
            drow[static_cast<size_t>(x) * 3] = clampByte(acc[0]);
            drow[static_cast<size_t>(x) * 3 + 1] = clampByte(acc[1]);
            drow[static_cast<size_t>(x) * 3 + 2] = clampByte(acc[2]);
        }
    }

    Image8 out(img.w(), img.h());
    for (int y = 0; y < img.h(); ++y) {
        uint8_t* drow = out.row(y);
        for (int x = 0; x < img.w(); ++x) {
            double acc[3] = {0.0, 0.0, 0.0};
            for (int k = 0; k < size; ++k) {
                const int sy = edgeIndex(y + k - r, img.h(), em);
                const uint8_t* p = tmp.row(sy) + static_cast<size_t>(x) * 3;
                acc[0] += g[static_cast<size_t>(k)] * p[0];
                acc[1] += g[static_cast<size_t>(k)] * p[1];
                acc[2] += g[static_cast<size_t>(k)] * p[2];
            }
            drow[static_cast<size_t>(x) * 3] = clampByte(acc[0]);
            drow[static_cast<size_t>(x) * 3 + 1] = clampByte(acc[1]);
            drow[static_cast<size_t>(x) * 3 + 2] = clampByte(acc[2]);
        }
    }
    return out;
}

Image8 laplacianSharpen(const Image8& img, int connectivity, double amount, EdgeMode em) {
    const Image8 lap = convolve(img, makeLaplacian(connectivity), em);
    Image8 out(img.w(), img.h());
    for (size_t i = 0; i < img.rgb.size(); ++i) {
        out.rgb[i] = clampByte(img.rgb[i] - amount * lap.rgb[i]);
    }
    return out;
}

Image8 unsharpMask(const Image8& img, int radius, double amount, EdgeMode em) {
    const Image8 blur = gaussianBlur(img, radius, em);
    Image8 out(img.w(), img.h());
    for (size_t i = 0; i < img.rgb.size(); ++i) {
        out.rgb[i] = clampByte(img.rgb[i] + amount * (img.rgb[i] - blur.rgb[i]));
    }
    return out;
}

}
