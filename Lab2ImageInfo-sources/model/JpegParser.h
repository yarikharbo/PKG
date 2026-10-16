#pragma once

#include "ByteReader.h"
#include "ImageInfo.h"

namespace img {

bool parseJpeg(ByteReader& r, ImageInfo& out);

}
