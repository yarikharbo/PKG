#pragma once

#include "Image8.h"

#include <vector>

namespace imgproc {

Image8 toGrayscaleBin(const Image8& img, int threshold);
int otsuThreshold(const std::vector<int>& hist);
Image8 thresholdOtsu(const Image8& img);
Image8 thresholdAdaptive(const Image8& img, int windowSize, int offsetC);

}
