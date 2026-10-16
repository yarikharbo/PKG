#pragma once

#include "ByteReader.h"
#include "ImageInfo.h"

namespace img {

bool parsePcx(ByteReader& r, ImageInfo& out);

}
