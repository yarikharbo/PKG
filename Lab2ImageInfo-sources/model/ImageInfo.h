#pragma once

#include <cstdint>
#include <string>

namespace img {

enum class Format { Unknown, BMP, PNG, GIF, JPEG, TIFF, PCX, NotImage };

struct ImageInfo {
    Format format = Format::Unknown;
    std::string fileName;
    std::string filePath;
    std::string formatName;
    uint64_t fileSize = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    double dpiX = 0.0;
    double dpiY = 0.0;
    bool hasResolution = false;
    uint32_t bitsPerPixel = 0;
    std::string bitDepthText;
    uint32_t compressionCode = 0;
    std::string compression;
    bool ok = false;
    std::string status;
    bool renamed = false;
    std::string extra;
};

const char* formatNameOf(Format f);
const char* extensionOf(Format f);

}
