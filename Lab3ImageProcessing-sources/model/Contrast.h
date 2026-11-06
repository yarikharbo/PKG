#pragma once

#include "Image8.h"

#include <array>
#include <vector>

namespace imgproc {

enum class EqualizeMode { Gray, PerChannelRGB, HSV_V };

std::vector<int> histogramGray(const Image8& img);
std::array<std::vector<int>, 3> histogramChannels(const Image8& img);

std::vector<uint8_t> linearContrastLUT(int blackPoint, int whitePoint);
std::vector<uint8_t> equalizeLUT(const std::vector<int>& hist);

Image8 applyLUT(const Image8& img, const std::vector<uint8_t>& lut);
Image8 linearContrast(const Image8& img, int blackPoint, int whitePoint);
Image8 equalizeGlobal(const Image8& img, EqualizeMode mode);
Image8 toGrayscale(const Image8& img);

}
