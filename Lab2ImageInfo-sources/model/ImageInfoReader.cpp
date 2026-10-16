#include "ImageInfoReader.h"

#include "BmpParser.h"
#include "ByteReader.h"
#include "GifParser.h"
#include "ImageInfo.h"
#include "JpegParser.h"
#include "PcxParser.h"
#include "PngParser.h"
#include "TiffParser.h"

#include <string>

namespace img {

namespace {

Format sniff(const uint8_t* b, size_t n) {
    if (n >= 2 && b[0] == 'B' && b[1] == 'M') return Format::BMP;
    if (n >= 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G' &&
        b[4] == 0x0D && b[5] == 0x0A && b[6] == 0x1A && b[7] == 0x0A) return Format::PNG;
    if (n >= 4 && b[0] == 'G' && b[1] == 'I' && b[2] == 'F' && b[3] == '8') return Format::GIF;
    if (n >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) return Format::JPEG;
    if (n >= 4 && ((b[0] == 0x49 && b[1] == 0x49 && b[2] == 0x2A && b[3] == 0x00) ||
                   (b[0] == 0x4D && b[1] == 0x4D && b[2] == 0x00 && b[3] == 0x2A))) return Format::TIFF;
    if (n >= 1 && b[0] == 0x0A) return Format::PCX;
    return Format::Unknown;
}

}

ImageInfo readImageInfo(const std::string& path) {
    ImageInfo info;
    const size_t slash = path.find_last_of("/\\");
    info.fileName = slash == std::string::npos ? path : path.substr(slash + 1);
    info.filePath = path;

    ByteReader r;
    if (!r.open(path)) {
        info.format = Format::NotImage;
        info.status = "не удалось открыть файл";
        return info;
    }
    info.fileSize = r.size();

    uint8_t head[16] = {};
    const size_t got = r.size() < 16 ? static_cast<size_t>(r.size()) : 16;
    if (got == 0 || !r.read(head, got)) {
        info.format = Format::NotImage;
        info.status = "пустой или нечитаемый файл";
        return info;
    }

    const Format f = sniff(head, got);
    const std::string ext = ByteReader::extensionOf(path);

    switch (f) {
    case Format::BMP: parseBmp(r, info); break;
    case Format::PNG: parsePng(r, info); break;
    case Format::GIF: parseGif(r, info); break;
    case Format::JPEG: parseJpeg(r, info); break;
    case Format::TIFF: parseTiff(r, info); break;
    case Format::PCX: parsePcx(r, info); break;
    default: {
        info.format = Format::NotImage;
        info.renamed = ext == "bmp" || ext == "png" || ext == "gif" || ext == "jpg" || ext == "jpeg" ||
                       ext == "tif" || ext == "tiff" || ext == "pcx";
        info.status = info.renamed
            ? "не изображение: содержимое не соответствует расширению ." + ext
            : "не поддерживаемый формат изображения";
        break;
    }
    }

    if (f != Format::Unknown && f != Format::NotImage) {
        const std::string actual = extensionOf(f);
        const bool alias = (actual == "jpg" && (ext == "jpg" || ext == "jpeg")) ||
                           (actual == "tif" && (ext == "tif" || ext == "tiff"));
        if (!ext.empty() && !alias && ext != actual) {
            info.renamed = true;
            if (info.status == "OK") {
                info.status = "OK (расширение ." + ext + " не совпадает с фактическим форматом)";
            }
        }
    }
    return info;
}

}
