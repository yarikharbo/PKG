#include "PngParser.h"

#include <string>

namespace img {

namespace {

const char* colorTypeName(uint8_t ct) {
    switch (ct) {
    case 0: return "градации серого";
    case 2: return "RGB (истинный цвет)";
    case 3: return "палитровый";
    case 4: return "серый + альфа";
    case 6: return "RGBA (истинный цвет + альфа)";
    default: return "неизвестный тип цвета";
    }
}

int channelsOf(uint8_t ct) {
    switch (ct) {
    case 0: return 1;
    case 2: return 3;
    case 3: return 1;
    case 4: return 2;
    case 6: return 4;
    default: return 0;
    }
}

}

bool parsePng(ByteReader& r, ImageInfo& out) {
    out.format = Format::PNG;
    out.formatName = "PNG";
    out.compression = "Deflate/Inflate (zlib) + фильтрация строк";

    const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    uint8_t got[8];
    if (!r.seek(0) || !r.read(got, 8)) {
        out.status = "повреждён: файл короче сигнатуры";
        return false;
    }
    for (int i = 0; i < 8; ++i) {
        if (got[i] != sig[i]) {
            out.status = "повреждён: неверная сигнатура PNG";
            return false;
        }
    }

    uint32_t len = 0;
    char type[5] = {};
    if (!r.u32be(len) || !r.read(reinterpret_cast<uint8_t*>(type), 4) || len != 13 || std::string(type, 4) != "IHDR") {
        out.status = "повреждён: первый чанк не IHDR";
        return false;
    }

    uint32_t w = 0, h = 0;
    uint8_t depth = 0, ct = 0, comp = 0, filter = 0, interlace = 0;
    if (!r.u32be(w) || !r.u32be(h) || !r.u8(depth) || !r.u8(ct) || !r.u8(comp) ||
        !r.u8(filter) || !r.u8(interlace)) {
        out.status = "повреждён: обрезан чанк IHDR";
        return false;
    }

    const int ch = channelsOf(ct);
    const bool depthOk = (ct == 3 ? depth <= 8 : depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16);
    if (w == 0 || h == 0 || ch == 0 || !depthOk || comp != 0 || filter != 0 || interlace > 1) {
        out.status = "повреждён: некорректные поля IHDR";
        return false;
    }

    out.width = w;
    out.height = h;
    out.bitsPerPixel = depth * static_cast<uint8_t>(ch);
    out.bitDepthText = std::to_string(depth) + " бит/канал";
    if (ch > 1) {
        out.bitDepthText += " x " + std::to_string(ch) + " канала";
        out.bitDepthText += ct == 4 ? " (серый + альфа)" : ct == 6 ? " (RGBA)" : " (RGB)";
        out.bitDepthText += " = " + std::to_string(out.bitsPerPixel) + " бит/пиксель";
    } else if (ct == 3) {
        out.bitDepthText += " (индекс палитры)";
    } else {
        out.bitDepthText += " (градации серого)";
    }

    if (!r.skip(4)) {
        out.status = "повреждён: обрезан CRC чанка IHDR";
        return false;
    }

    bool iendSeen = false;
    bool plteSeen = false;
    uint32_t plteEntries = 0;
    uint64_t pos = 33;
    while (pos + 12 <= r.size()) {
        if (!r.seek(pos) || !r.u32be(len) || !r.read(reinterpret_cast<uint8_t*>(type), 4)) {
            out.status = "повреждён: обрезан заголовок чанка";
            return false;
        }
        std::string t(type, 4);
        if (t == "IEND") {
            iendSeen = true;
            break;
        }
        if (t == "PLTE") {
            plteSeen = true;
            plteEntries = len / 3;
        }
        if (t == "pHYs" && len == 9) {
            uint32_t ppuX = 0, ppuY = 0;
            uint8_t unit = 0;
            if (!r.u32be(ppuX) || !r.u32be(ppuY) || !r.u8(unit)) {
                out.status = "повреждён: обрезан чанк pHYs";
                return false;
            }
            if (unit == 1 && (ppuX > 0 || ppuY > 0)) {
                out.hasResolution = true;
                out.dpiX = ppuX * 0.0254;
                out.dpiY = ppuY * 0.0254;
            }
        }
        const uint64_t next = pos + 12ull + len;
        if (next > r.size() || len > 0x7FFFFFFFull) {
            out.status = "повреждён: чанк выходит за пределы файла";
            return false;
        }
        pos = next;
    }

    if (!iendSeen) {
        out.status = "повреждён: отсутствует завершающий чанк IEND";
        return false;
    }

    out.extra = "Тип цвета: ";
    out.extra += colorTypeName(ct);
    if (plteSeen) {
        out.extra += "\nПалитра (чанк PLTE): ";
        out.extra += std::to_string(plteEntries);
        out.extra += " записей RGB (только для палитрового типа; для остальных — предлагаемая палитра)";
    } else if (ct == 3) {
        out.extra += "\nПалитра: чанк PLTE отсутствует (файл повреждён)";
    }
    out.extra += "\nМежстрочная развёртка: ";
    out.extra += interlace == 1 ? "Adam7 (7 проходов)" : "нет (sequential)";
    out.extra += "\nМетод сжатия: Deflate (zlib), метод фильтрации: ";
    out.extra += std::to_string(filter);
    out.ok = true;
    out.status = "OK";
    return true;
}

}
