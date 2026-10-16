#pragma once

#include "ByteReader.h"
#include "ImageInfo.h"

namespace img {

bool parseTiff(ByteReader& r, ImageInfo& out);

}
