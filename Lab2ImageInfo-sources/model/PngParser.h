#pragma once

#include "ByteReader.h"
#include "ImageInfo.h"

namespace img {

bool parsePng(ByteReader& r, ImageInfo& out);

}
