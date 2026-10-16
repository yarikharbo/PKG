#include "GifParser.h"

#include <string>

namespace img {

bool parseGif(ByteReader& r, ImageInfo& out) {
    out.format = Format::GIF;
    out.formatName = "GIF";
    out.compressionCode = 5;
    out.compression = "LZW с переменной длиной кода";

    char ver[6] = {};
    if (!r.seek(0) || !r.read(reinterpret_cast<uint8_t*>(ver), 6) || ver[0] != 'G' || ver[1] != 'I' || ver[2] != 'F' || ver[3] != '8') {
        out.status = "повреждён: нет сигнатуры GIF8";
        return false;
    }
    const std::string version(ver + 3, 3);
    if (version != "87a" && version != "89a") {
        out.status = "повреждён: неизвестная версия GIF";
        return false;
    }

    uint16_t w = 0, h = 0;
    uint8_t flags = 0, bg = 0, aspect = 0;
    if (!r.u16le(w) || !r.u16le(h) || !r.u8(flags) || !r.u8(bg) || !r.u8(aspect)) {
        out.status = "повреждён: обрезан дескриптор логического экрана";
        return false;
    }
    if (w == 0 || h == 0) {
        out.status = "повреждён: нулевые размеры логического экрана";
        return false;
    }

    const uint8_t gctBpp = (flags & 0x07) + 1;
    const uint32_t gctColors = (flags & 0x80) ? (1u << gctBpp) : 0;
    uint64_t pos = 13 + (gctColors ? static_cast<uint64_t>(gctColors) * 3 : 0);
    if (pos > r.size()) {
        out.status = "повреждён: глобальная палитра выходит за пределы файла";
        return false;
    }

    uint32_t images = 0;
    uint32_t maxBpp = (flags & 0x80) ? gctBpp : 0;
    bool trailerSeen = false;
    while (pos < r.size()) {
        if (!r.seek(pos)) break;
        uint8_t kind = 0;
        if (!r.u8(kind)) break;
        if (kind == 0x3B) {
            trailerSeen = true;
            pos += 1;
            break;
        }
        if (kind == 0x21) {
            if (!r.skip(1)) break;
            uint8_t sub = 0;
            uint64_t p = r.tell();
            bool ok = true;
            while (true) {
                if (!r.seek(p) || !r.u8(sub)) { ok = false; break; }
                if (sub == 0) { p += 1; break; }
                p += 1 + sub;
                if (p > r.size()) { ok = false; break; }
            }
            if (!ok) break;
            pos = p;
            continue;
        }
        if (kind == 0x2C) {
            uint8_t lflags = 0;
            if (!r.skip(8) || !r.u8(lflags)) break;
            const uint8_t lctBpp = (lflags & 0x07) + 1;
            if (lflags & 0x80) {
                pos = r.tell() + (1ull << lctBpp) * 3;
                if (lctBpp > maxBpp) maxBpp = lctBpp;
            } else {
                pos = r.tell();
            }
            if (pos > r.size()) break;
            if (!r.seek(pos)) break;
            uint8_t minCodeSize = 0;
            if (!r.u8(minCodeSize)) break;
            pos += 1;
            if (minCodeSize > 11) break;
            uint8_t sub = 0;
            bool ok = true;
            while (true) {
                if (!r.seek(pos) || !r.u8(sub)) { ok = false; break; }
                if (sub == 0) { pos += 1; break; }
                pos += 1 + sub;
                if (pos > r.size()) { ok = false; break; }
            }
            if (!ok) break;
            ++images;
            continue;
        }
        break;
    }

    if (!trailerSeen) {
        out.status = "повреждён: отсутствует завершитель 0x3B (или обрезаны блоки данных)";
        out.width = w;
        out.height = h;
        out.bitsPerPixel = maxBpp;
        out.bitDepthText = std::to_string(maxBpp) + " бит/пиксель (палитровый)";
        return false;
    }

    if (maxBpp == 0) maxBpp = 1;
    out.width = w;
    out.height = h;
    out.bitsPerPixel = maxBpp;
    out.bitDepthText = std::to_string(maxBpp) + " бит/пиксель (палитровый)";
    out.extra = "Версия GIF: " + version;
    out.extra += "\nГлобальная палитра: ";
    out.extra += gctColors ? std::to_string(gctColors) + " цветов" : "не задана";
    out.extra += "\nКадров в файле: " + std::to_string(images);
    out.extra += "\nАнимация: ";
    out.extra += images > 1 ? "да" : "нет";
    if (version == "89a") {
        out.extra += "\nЦвет фона (индекс в палитре): " + std::to_string(bg);
    }
    out.ok = true;
    out.status = "OK";
    return true;
}

}
