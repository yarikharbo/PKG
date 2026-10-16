#pragma once

#include "ByteReader.h"
#include "ImageInfo.h"

namespace img {

bool parseBmp(ByteReader& r, ImageInfo& out);

}
