#include "BmpParser.h"

#include <string>

namespace img {

namespace {

const char* compressionName(uint32_t c) {
    switch (c) {
    case 0: return "без сжатия (BI_RGB)";
    case 1: return "RLE 8 бит/пиксель (BI_RLE8)";
    case 2: return "RLE 4 бита/пиксель (BI_RLE4)";
    case 3: return "битовые маски (BI_BITFIELDS)";
    case 4: return "вложенный JPEG (BI_JPEG)";
    case 5: return "вложенный PNG (BI_PNG)";
    case 6: return "битовые маски + альфа (BI_ALPHABITFIELDS)";
    default: return "неизвестный код сжатия";
    }
}

const char* headerName(uint32_t hsz) {
    if (hsz == 12) return "BITMAPCOREHEADER";
    if (hsz == 40) return "BITMAPINFOHEADER";
    if (hsz == 52) return "BITMAPV2INFOHEADER";
    if (hsz == 56) return "BITMAPV3INFOHEADER";
    if (hsz == 64) return "OS22XBITMAPHEADER";
    if (hsz == 108) return "BITMAPV4HEADER";
    if (hsz == 124) return "BITMAPV5HEADER";
    return "нестандартный заголовок";
    }

}

bool parseBmp(ByteReader& r, ImageInfo& out) {
    out.format = Format::BMP;
    out.formatName = "BMP";
    out.compressionCode = 0;
    out.compression = compressionName(0);

    uint8_t magic[2];
    if (!r.seek(0) || !r.read(magic, 2) || magic[0] != 'B' || magic[1] != 'M') {
        out.status = "повреждён: нет сигнатуры BM";
        return false;
    }

    uint32_t declaredSize = 0, offBits = 0;
    uint16_t res1 = 0, res2 = 0;
    if (!r.u32le(declaredSize) || !r.u16le(res1) || !r.u16le(res2) || !r.u32le(offBits)) {
        out.status = "повреждён: обрезан заголовок файла";
        return false;
    }

    uint32_t hsz = 0;
    if (!r.u32le(hsz) || hsz < 12) {
        out.status = "повреждён: обрезан или некорректен инфо-заголовок";
        return false;
    }
    if (static_cast<uint64_t>(14) + hsz > r.size()) {
        out.status = "повреждён: заголовок больше файла";
        return false;
    }

    uint32_t w = 0, h = 0, comp = 0, imgSize = 0, xppm = 0, yppm = 0, clrUsed = 0, clrImp = 0;
    uint16_t planes = 0, bpp = 0;

    if (hsz == 12) {
        uint16_t w16 = 0, h16 = 0;
        if (!r.u16le(w16) || !r.u16le(h16) || !r.u16le(planes) || !r.u16le(bpp)) {
            out.status = "повреждён: обрезан BITMAPCOREHEADER";
            return false;
        }
        w = w16;
        h = h16;
        comp = 0;
    } else {
        int32_t ws = 0, hs = 0, xs = 0, ys = 0;
        if (!r.s32le(ws) || !r.s32le(hs) || !r.u16le(planes) || !r.u16le(bpp) ||
            !r.u32le(comp) || !r.u32le(imgSize) || !r.s32le(xs) || !r.s32le(ys) ||
            !r.u32le(clrUsed) || !r.u32le(clrImp)) {
            out.status = "повреждён: обрезан BITMAPINFOHEADER";
            return false;
        }
        w = static_cast<uint32_t>(ws > 0 ? ws : 0);
        h = static_cast<uint32_t>(hs < 0 ? -static_cast<long long>(hs) : hs);
        xppm = static_cast<uint32_t>(xs > 0 ? xs : 0);
        yppm = static_cast<uint32_t>(ys > 0 ? ys : 0);
    }

    if (w == 0 || h == 0) {
        out.status = "повреждён: нулевые размеры изображения";
        return false;
    }
    const bool knownBpp = bpp == 1 || bpp == 4 || bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32 || bpp == 64;
    if (!knownBpp || planes == 0 || planes > 32) {
        out.status = "повреждён: некорректные битовая глубина или число плоскостей";
        return false;
    }
    if (offBits > r.size()) {
        out.status = "повреждён: смещение данных за пределами файла";
        return false;
    }

    if (comp == 0 && hsz >= 40) {
        const uint64_t rowSize = (static_cast<uint64_t>(w) * bpp + 31) / 32 * 4;
        const uint64_t need = offBits + rowSize * h;
        if (need > r.size()) {
            out.status = "повреждён: файл меньше, чем заявлено в заголовке";
            return false;
        }
    }
    if (declaredSize != 0 && declaredSize > r.size()) {
        out.status = "повреждён: заявленный размер файла больше фактического";
        return false;
    }

    out.width = w;
    out.height = h;
    out.bitsPerPixel = bpp;
    out.bitDepthText = bpp == 24 ? "24 бита/пиксель (RGB)"
                     : bpp == 32 ? "32 бита/пиксель (RGBA или битовые маски)"
                     : bpp == 16 ? "16 бит/пиксель (High Color)"
                     : bpp == 64 ? "64 бита/пиксель"
                     : bpp == 8 ? "8 бит/пиксель (индекс палитры)"
                     : bpp == 4 ? "4 бита/пиксель (индекс палитры)"
                                : "1 бит/пиксель (монохром)";
    out.compressionCode = comp;
    out.compression = compressionName(comp);

    if (xppm > 0 || yppm > 0) {
        out.hasResolution = true;
        out.dpiX = xppm * 0.0254;
        out.dpiY = yppm * 0.0254;
    }

    uint32_t palette = clrUsed;
    if (palette == 0 && bpp <= 8) palette = 1u << bpp;
    out.extra = "Тип заголовка: ";
    out.extra += headerName(hsz);
    if (bpp <= 8) {
        out.extra += "\nПалитра: таблица из ";
        out.extra += std::to_string(palette);
        out.extra += " цветов (RGBA-записи по 4 байта) между заголовком и пиксельными данными";
    } else {
        out.extra += "\nПалитра: не используется (пиксели хранят цвет напрямую)";
    }
    if (hsz >= 40 && (comp == 1 || comp == 2)) {
        out.extra += "\nДанные сжаты построчным RLE: пары «счётчик, значение»";
    }
    out.extra += "\nПорядок строк: снизу вверх (первая строка данных — нижняя)";
    out.ok = true;
    out.status = "OK";
    return true;
}

}
