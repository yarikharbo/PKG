#include "ImageInfo.h"

namespace img {

const char* formatNameOf(Format f) {
    switch (f) {
    case Format::BMP: return "BMP";
    case Format::PNG: return "PNG";
    case Format::GIF: return "GIF";
    case Format::JPEG: return "JPEG";
    case Format::TIFF: return "TIFF";
    case Format::PCX: return "PCX";
    case Format::NotImage: return "—";
    default: return "?";
    }
}

const char* extensionOf(Format f) {
    switch (f) {
    case Format::BMP: return "bmp";
    case Format::PNG: return "png";
    case Format::GIF: return "gif";
    case Format::JPEG: return "jpg";
    case Format::TIFF: return "tif";
    case Format::PCX: return "pcx";
    default: return "";
    }
}

}
