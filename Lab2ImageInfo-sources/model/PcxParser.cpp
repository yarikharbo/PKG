#include "PcxParser.h"

#include <string>

namespace img {

namespace {

const char* versionName(uint8_t v) {
    switch (v) {
    case 0: return "0 (Paintbrush 2.5, монохромная палитра)";
    case 2: return "2 (Paintbrush 2.8, палитра)";
    case 3: return "3 (Paintbrush 2.8, без палитры)";
    case 4: return "4 (PC Paintbrush for Windows)";
    case 5: return "5 (Paintbrush 3.0+, полная палитра)";
    default: return "неизвестная версия";
    }
}

}

bool parsePcx(ByteReader& r, ImageInfo& out) {
    out.format = Format::PCX;
    out.formatName = "PCX";
    out.compressionCode = 1;
    out.compression = "RLE по строкам (пары «счётчик-191, байт»)";

    uint8_t hdr[128];
    if (!r.seek(0) || !r.read(hdr, 128)) {
        out.status = "повреждён: файл короче 128-байтного заголовка";
        return false;
    }
    const uint8_t magic = hdr[0], version = hdr[1], encoding = hdr[2], bppPlane = hdr[3];
    const int xmin = hdr[4] | (hdr[5] << 8), ymin = hdr[6] | (hdr[7] << 8);
    const int xmax = hdr[8] | (hdr[9] << 8), ymax = hdr[10] | (hdr[11] << 8);
    const int hres = hdr[12] | (hdr[13] << 8), vres = hdr[14] | (hdr[15] << 8);
    const uint8_t nplanes = hdr[65];
    const int bytesPerLine = hdr[66] | (hdr[67] << 8);
    const uint16_t paletteType = static_cast<uint16_t>(hdr[68] | (hdr[69] << 8));
    const uint16_t hscreen = static_cast<uint16_t>(hdr[70] | (hdr[71] << 8));
    const uint16_t vscreen = static_cast<uint16_t>(hdr[72] | (hdr[73] << 8));

    if (magic != 0x0A || version > 5) {
        out.status = "повреждён: неверный байт-производитель или версия";
        return false;
    }
    if (encoding > 1) {
        out.status = "повреждён: неизвестный метод кодирования";
        return false;
    }
    if (nplanes < 1 || nplanes > 4 || (bppPlane != 1 && bppPlane != 4 && bppPlane != 8)) {
        out.status = "повреждён: некорректные плоскости или глубина";
        return false;
    }
    const int w = xmax - xmin + 1;
    const int h = ymax - ymin + 1;
    if (w <= 0 || h <= 0 || bytesPerLine <= 0 || bytesPerLine > 8192) {
        out.status = "повреждён: некорректные координаты габаритов";
        return false;
    }

    out.width = static_cast<uint32_t>(w);
    out.height = static_cast<uint32_t>(h);
    out.bitsPerPixel = static_cast<uint32_t>(bppPlane) * nplanes;
    out.bitDepthText = std::to_string(bppPlane) + " бит/плоскость x " + std::to_string(nplanes) +
                       " плоскост" + (nplanes == 1 ? "ь" : "и") + " = " + std::to_string(out.bitsPerPixel) +
                       " бит/пиксель" +
                       (out.bitsPerPixel <= 8 ? " (палитровый режим)" : " (истинный цвет)");
    out.compressionCode = encoding;
    out.compression = encoding == 1 ? "RLE по строкам (пары «счётчик-191, байт»)" : "без сжатия";

    if (hres > 0 && vres > 0) {
        out.hasResolution = true;
        out.dpiX = hres;
        out.dpiY = vres;
    }

    uint32_t paletteColors = 0;
    if (out.bitsPerPixel <= 4) {
        paletteColors = 1u << out.bitsPerPixel;
    } else if (version == 5 && bppPlane == 8 && nplanes == 1 && r.size() >= 128 + 769) {
        uint8_t marker = 0;
        if (r.seek(r.size() - 769) && r.u8(marker) && marker == 0x0C) {
            paletteColors = 256;
        }
    }

    out.extra = "Версия формата: ";
    out.extra += versionName(version);
    out.extra += "\nБайтов на строку (с выравниванием на чётное): ";
    out.extra += std::to_string(bytesPerLine);
    out.extra += "\nГабариты экрана источника: ";
    out.extra += hscreen && vscreen ? std::to_string(hscreen) + " x " + std::to_string(vscreen) : "не заданы";
    out.extra += "\nТип палитры: ";
    out.extra += paletteType == 1 ? "цветная" : paletteType == 2 ? "градации серого" : "не задан";
    out.extra += "\nПалитра: ";
    out.extra += paletteColors ? (paletteColors == 256 ? "256 цветов (RGB-триплеты в конце файла после байта 0x0C)" : std::to_string(paletteColors) + " цветов (в заголовке, 16 x RGB)")
                               : "не используется (истинный цвет)";
    out.extra += "\nРазрешение (hres/vres из заголовка): ";
    out.extra += out.hasResolution ? std::to_string(hres) + " x " + std::to_string(vres) + " dpi (DPI устройства-источника)" : "не задано";
    out.ok = true;
    out.status = "OK";
    return true;
}

}
