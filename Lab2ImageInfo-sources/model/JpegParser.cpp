#include "JpegParser.h"

#include <cstring>
#include <string>
#include <vector>

namespace img {

namespace {

bool isSofMarker(uint8_t m) {
    return m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC;
}

const char* sofName(uint8_t m) {
    switch (m) {
    case 0xC0: return "baseline DCT (последовательный, код Хаффмана)";
    case 0xC1: return "extended sequential DCT, Huffman";
    case 0xC2: return "progressive DCT, Huffman";
    case 0xC3: return "lossless, Huffman";
    case 0xC5: return "differential sequential DCT, Huffman";
    case 0xC6: return "differential progressive DCT, Huffman";
    case 0xC7: return "differential lossless, Huffman";
    case 0xC9: return "sequential DCT, арифметическое кодирование";
    case 0xCA: return "progressive DCT, арифметическое кодирование";
    case 0xCB: return "lossless, арифметическое кодирование";
    case 0xCD: return "differential sequential DCT, арифметическое";
    case 0xCE: return "differential progressive DCT, арифметическое";
    case 0xCF: return "differential lossless, арифметическое";
    default: return "неизвестный SOF";
    }
}

bool parseExifResolution(ByteReader& r, uint64_t tiffStart, double& dpiX, double& dpiY) {
    uint16_t bo = 0;
    if (!r.seek(tiffStart) || !r.u16le(bo)) return false;
    const bool little = bo == 0x4949;
    uint16_t magic = 0;
    if (little ? !r.u16le(magic) : !r.u16be(magic)) return false;
    if (magic != 42) return false;
    uint32_t ifdOff = 0;
    if (little ? !r.u32le(ifdOff) : !r.u32be(ifdOff)) return false;
    const uint64_t ifd = tiffStart + ifdOff;
    if (ifd + 2 > r.size() || ifd < tiffStart) return false;
    if (!r.seek(ifd)) return false;
    uint16_t count = 0;
    if (little ? !r.u16le(count) : !r.u16be(count)) return false;
    if (static_cast<uint64_t>(count) * 12 + 6 > r.size() - ifd) return false;

    double xres = 0, yres = 0;
    uint16_t unit = 2;
    for (uint16_t i = 0; i < count; ++i) {
        if (!r.seek(ifd + 2 + static_cast<uint64_t>(i) * 12)) return false;
        uint16_t tag = 0, type = 0;
        uint32_t cnt = 0, val = 0;
        if (little ? !r.u16le(tag) : !r.u16be(tag)) return false;
        if (little ? !r.u16le(type) : !r.u16be(type)) return false;
        if (little ? !r.u32le(cnt) : !r.u32be(cnt)) return false;
        if (little ? !r.u32le(val) : !r.u32be(val)) return false;
        if (type != 5 || cnt != 1) continue;
        const uint64_t vp = tiffStart + val;
        if (vp + 8 > r.size() || vp < tiffStart) continue;
        uint32_t num = 0, den = 0;
        if (!r.seek(vp)) continue;
        if (little ? (!r.u32le(num) || !r.u32le(den)) : (!r.u32be(num) || !r.u32be(den))) continue;
        if (den == 0) continue;
        const double v = static_cast<double>(num) / den;
        if (tag == 0x11A) xres = v;
        else if (tag == 0x11B) yres = v;
    }
    for (uint16_t i = 0; i < count; ++i) {
        if (!r.seek(ifd + 2 + static_cast<uint64_t>(i) * 12)) break;
        uint16_t tag = 0, type = 0;
        uint32_t cnt = 0, val = 0;
        if (little ? !r.u16le(tag) : !r.u16be(tag)) break;
        if (little ? !r.u16le(type) : !r.u16be(type)) break;
        if (little ? !r.u32le(cnt) : !r.u32be(cnt)) break;
        if (little ? !r.u32le(val) : !r.u32be(val)) break;
        if (tag == 0x128 && type == 3) unit = little ? static_cast<uint16_t>(val & 0xFFFF) : static_cast<uint16_t>(val >> 16);
    }
    if (xres <= 0 && yres <= 0) return false;
    const double k = unit == 3 ? 2.54 : 1.0;
    dpiX = xres * k;
    dpiY = (yres > 0 ? yres : xres) * k;
    return true;
}

}

bool parseJpeg(ByteReader& r, ImageInfo& out) {
    out.format = Format::JPEG;
    out.formatName = "JPEG";

    uint16_t soi = 0;
    if (!r.seek(0) || !r.u16be(soi) || soi != 0xFFD8) {
        out.status = "повреждён: нет маркера SOI";
        return false;
    }

    bool sofSeen = false;
    uint8_t sofMarker = 0, precision = 0, nComp = 0;
    uint16_t sofW = 0, sofH = 0;
    uint8_t jfifUnit = 0; uint16_t jfifX = 0, jfifY = 0;
    bool jfifSeen = false, exifSeen = false, dhtSeen = false, dacSeen = false;
    std::string dqtText;

    uint64_t pos = 2;
    while (pos + 4 <= r.size()) {
        if (!r.seek(pos)) break;
        uint8_t ff = 0, m = 0;
        if (!r.u8(ff) || !r.u8(m)) break;
        if (ff != 0xFF) { pos += 1; continue; }
        if (m == 0xFF) { pos += 1; continue; }
        if (m == 0xD8) { pos += 2; continue; }
        if (m == 0xD9) break;
        if (m == 0x01 || (m >= 0xD0 && m <= 0xD7)) { pos += 2; continue; }

        uint16_t segLen = 0;
        if (pos + 4 > r.size()) break;
        if (!r.seek(pos + 2) || !r.u16be(segLen) || segLen < 2) {
            out.status = "повреждён: некорректная длина сегмента";
            return false;
        }

        if (isSofMarker(m)) {
            const uint64_t p = pos + 4;
            if (p + 6 > r.size()) break;
            uint8_t pr = 0, nc = 0;
            uint16_t h = 0, w = 0;
            if (!r.seek(p) || !r.u8(pr) || !r.u16be(h) || !r.u16be(w) || !r.u8(nc)) break;
            if (!sofSeen && w > 0 && h > 0 && nc > 0 && nc <= 4) {
                sofSeen = true;
                sofMarker = m;
                precision = pr;
                nComp = nc;
                sofW = w;
                sofH = h;
            }
        } else if (m == 0xE0 && !jfifSeen && segLen >= 14) {
            const uint64_t p = pos + 4;
            if (p + 12 <= r.size()) {
                char id[5] = {};
                uint8_t majorV = 0, minorV = 0;
                if (r.seek(p) && r.read(reinterpret_cast<uint8_t*>(id), 5) &&
                    std::memcmp(id, "JFIF\0", 5) == 0 && r.u8(majorV) && r.u8(minorV) &&
                    r.u8(jfifUnit) && r.u16be(jfifX) && r.u16be(jfifY)) {
                    jfifSeen = true;
                }
            }
        } else if (m == 0xE1 && !exifSeen && segLen >= 10) {
            const uint64_t p = pos + 4;
            if (p + 8 <= r.size()) {
                char id[6] = {};
                if (r.seek(p) && r.read(reinterpret_cast<uint8_t*>(id), 6) && std::memcmp(id, "Exif\0\0", 6) == 0) {
                    exifSeen = true;
                    double ex = 0, ey = 0;
                    if (parseExifResolution(r, p + 6, ex, ey)) {
                        if (!out.hasResolution) {
                            out.hasResolution = true;
                            out.dpiX = ex;
                            out.dpiY = ey;
                        }
                    }
                }
            }
        } else if (m == 0xDB && dqtText.empty()) {
            const uint64_t p = pos + 4;
            uint64_t end = pos + 2 + segLen;
            uint64_t q = p;
            int tables = 0;
            while (q < end && tables < 4) {
                if (!r.seek(q)) break;
                uint8_t pqTq = 0;
                if (!r.u8(pqTq)) break;
                const int pq = pqTq >> 4;
                const int tq = pqTq & 0x0F;
                const size_t n = pq ? 128 : 64;
                if (q + 1 + n > end) break;
                std::vector<uint8_t> vals(n);
                if (!r.read(vals.data(), n)) break;
                if (tables == 0) {
                    dqtText = "Таблица квантования #";
                    dqtText += std::to_string(tq);
                    dqtText += pq ? " (16-битовые элементы)" : " (8-битовые элементы)";
                    dqtText += ", зигзаг-порядок:\n";
                    for (int row = 0; row < 8; ++row) {
                        for (int col = 0; col < 8; ++col) {
                            char buf[8];
                            std::snprintf(buf, sizeof(buf), "%4u", vals[row * 8 + col]);
                            dqtText += buf;
                        }
                        dqtText += "\n";
                    }
                }
                q += 1 + n;
                ++tables;
            }
        } else if (m == 0xC4) {
            dhtSeen = true;
        } else if (m == 0xCC) {
            dacSeen = true;
        }

        if (m == 0xDA) break;
        const uint64_t next = pos + 2 + segLen;
        if (next <= pos) break;
        pos = next;
    }

    bool eoiSeen = false;
    {
        const uint64_t tail = r.size() < 65536 ? r.size() : 65536;
        const uint64_t start = r.size() - tail;
        std::vector<uint8_t> buf(static_cast<size_t>(tail));
        if (tail == 0 || (r.seek(start) && r.read(buf.data(), static_cast<size_t>(tail)))) {
            for (size_t i = buf.size(); i >= 2; --i) {
                if (buf[i - 2] == 0xFF && buf[i - 1] == 0xD9) { eoiSeen = true; break; }
            }
        }
    }

    if (!sofSeen) {
        out.status = eoiSeen ? "повреждён: не найден кадр SOF" : "повреждён: нет маркера EOI и кадра SOF";
        return false;
    }
    if (!eoiSeen) {
        out.status = "повреждён: отсутствует маркер EOI (файл обрезан)";
        out.width = sofW;
        out.height = sofH;
    } else {
        out.width = sofW;
        out.height = sofH;
    }
    out.bitsPerPixel = precision * nComp;
    out.bitDepthText = std::to_string(precision) + " бит/канал";
    if (nComp == 3) out.bitDepthText += " x 3 (YCbCr) = " + std::to_string(out.bitsPerPixel) + " бит/пиксель";
    else if (nComp == 4) out.bitDepthText += " x 4 (CMYK/YCCK) = " + std::to_string(out.bitsPerPixel) + " бит/пиксель";
    else if (nComp == 1) out.bitDepthText += " (градации серого)";
    out.compressionCode = sofMarker;
    out.compression = "JPEG (DCT): " + std::string(sofName(sofMarker));

    if (jfifSeen && jfifUnit > 0 && (jfifX > 0 || jfifY > 0) && !out.hasResolution) {
        out.hasResolution = true;
        out.dpiX = jfifUnit == 2 ? jfifX * 2.54 : jfifX;
        out.dpiY = jfifUnit == 2 ? jfifY * 2.54 : jfifY;
    }

    out.extra = "Процесс кодирования: ";
    out.extra += sofName(sofMarker);
    out.extra += dacSeen && !dhtSeen ? ", арифметическое кодирование" : ", кодирование Хаффмана (таблицы DHT)";
    out.extra += "\nКомпонент в кадре: ";
    out.extra += std::to_string(nComp);
    out.extra += nComp == 3 ? " (Y, Cb, Cr — субдискретизация хранится в SOF)" : nComp == 1 ? " (Y)" : "";
    out.extra += "\nИсточник разрешения: ";
    out.extra += out.hasResolution ? (exifSeen && !jfifSeen ? "EXIF (APP1, теги XResolution/YResolution)" : "JFIF (APP0: плотность и единицы)")
                                   : "не задано (JFIF unit = 0)";
    if (!dqtText.empty()) {
        out.extra += "\nМатрица квантования (маркер DQT) — шкала качества сжатия:\n";
        out.extra += dqtText;
    }
    out.ok = eoiSeen;
    if (eoiSeen) out.status = "OK";
    return eoiSeen;
}

}
