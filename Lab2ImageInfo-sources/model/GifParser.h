#pragma once

#include "ByteReader.h"
#include "ImageInfo.h"

namespace img {

bool parseGif(ByteReader& r, ImageInfo& out);

}
