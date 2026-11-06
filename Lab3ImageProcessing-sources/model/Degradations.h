#pragma once

#include "Image8.h"

namespace imgproc {

Image8 addGaussianNoise(const Image8& img, double sigma, unsigned seed);
Image8 addSaltPepper(const Image8& img, double probability, unsigned seed);
Image8 squeezeContrast(const Image8& img, int lowLevel, int highLevel);

}
