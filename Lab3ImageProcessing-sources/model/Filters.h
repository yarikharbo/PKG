#pragma once

#include "Image8.h"

#include <vector>

namespace imgproc {

enum class EdgeMode { Clamp, Reflect };

using Kernel1D = std::vector<double>;
struct Kernel2D {
    int size = 0;
    std::vector<double> data;
};

Kernel2D makeLaplacian(int connectivity);
Kernel1D makeGaussian1D(int radius, double sigma);
Kernel1D makeBox1D(int radius);

Image8 convolve(const Image8& img, const Kernel2D& k, EdgeMode em);
Image8 boxBlur(const Image8& img, int radius, EdgeMode em);
Image8 gaussianBlur(const Image8& img, int radius, EdgeMode em);
Image8 laplacianSharpen(const Image8& img, int connectivity, double amount, EdgeMode em);
Image8 unsharpMask(const Image8& img, int radius, double amount, EdgeMode em);

}
