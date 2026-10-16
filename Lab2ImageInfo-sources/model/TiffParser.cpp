#include "TiffParser.h"

#include <string>
#include <vector>

namespace img {

namespace {

const char* compressionName(uint32_t c) {
    switch (c) {
    case 1: return "без сжатия";
    case 2: return "CCITT Group 3 (модифицированный Хаффман)";
    case 3: return "CCITT Group 3 fax (1D)";
    case 4: return "CCITT Group 4 fax";
    case 5: return "LZW";
    case 6: return "JPEG (старый, 6.0)";
    case 7: return "JPEG (новый, JFIF)";
    case 8: return "Deflate (Adobe)";
    case 9: return "JBIG на чёрно-белых";
    case 10: return "JBIG на цветных";
    case 32766: return "Next 2-bit RLE";
    case 32773: return "PackBits (RLE по байтам)";
    case 32946: return "Deflate (PKZIP)";
    case 34712: return "JPEG 2000";
    default: return "неизвестный код сжатия";
    }
}

const char* photometricName(uint32_t p) {
    switch (p) {
    case 0: return "WhiteIsZero (монохром, инвертирован)";
    case 1: return "BlackIsZero (монохром)";
    case 2: return "RGB";
    case 3: return "палитровый";
    case 4: return "прозрачная маска";
    case 5: return "CMYK";
    case 6: return "YCbCr";
    case 8: return "CIELab";
    case 9: return "ICCLab";
    default: return "другая фотометрическая интерпретация";
    }
}

}

bool parseTiff(ByteReader& r, ImageInfo& out) {
    out.format = Format::TIFF;
    out.formatName = "TIFF";
    out.compression = "без сжатия";

    uint8_t hdr4[4];
    if (!r.seek(0) || !r.read(hdr4, 4)) {
        out.status = "повреждён: обрезан заголовок";
        return false;
    }
    bool little = false;
    if (hdr4[0] == 0x49 && hdr4[1] == 0x49 && hdr4[2] == 0x2A && hdr4[3] == 0x00) {
        little = true;
    } else if (hdr4[0] == 0x4D && hdr4[1] == 0x4D && hdr4[2] == 0x00 && hdr4[3] == 0x2A) {
        little = false;
    } else {
        out.status = "повреждён: неверный порядок байтов или число 42";
        return false;
    }

    auto u16 = [&](uint16_t& v) { return little ? r.u16le(v) : r.u16be(v); };
    auto u32 = [&](uint32_t& v) { return little ? r.u32le(v) : r.u32be(v); };

    uint32_t ifdOff = 0;
    if (!u32(ifdOff)) {
        out.status = "повреждён: обрезано смещение IFD";
        return false;
    }

    uint32_t ifds = 0;
    uint32_t w = 0, h = 0;
    std::vector<uint32_t> bps;
    uint32_t spp = 1, comp = 1, photo = 1;
    double xres = 0, yres = 0;
    uint16_t unit = 2;

    uint32_t cur = ifdOff;
    for (int guard = 0; guard < 1024 && cur != 0; ++guard) {
        if (cur < 8 || static_cast<uint64_t>(cur) + 6 > r.size()) {
            out.status = "повреждён: смещение IFD за пределами файла";
            return false;
        }
        if (!r.seek(cur)) {
            out.status = "повреждён: не читается IFD";
            return false;
        }
        uint16_t count = 0;
        if (!u16(count)) {
            out.status = "повреждён: обрезано число записей IFD";
            return false;
        }
        if (static_cast<uint64_t>(cur) + 6 + static_cast<uint64_t>(count) * 12 > r.size()) {
            out.status = "повреждён: записи IFD выходят за пределы файла";
            return false;
        }

        uint32_t pw = 0, ph = 0, pspp = 0, pcomp = 0, pphoto = 0;
        std::vector<uint32_t> pbps;
        double pxres = 0, pyres = 0;
        uint16_t punit = 2;

        for (uint16_t i = 0; i < count; ++i) {
            if (!r.seek(cur + 2 + static_cast<uint64_t>(i) * 12)) {
                out.status = "повреждён: не читается запись IFD";
                return false;
            }
            uint16_t tag = 0, type = 0;
            uint32_t cnt = 0, val = 0;
            if (!u16(tag) || !u16(type) || !u32(cnt) || !u32(val)) {
                out.status = "повреждён: обрезана запись IFD";
                return false;
            }
            const size_t typeSize = type == 3 ? 2 : type == 4 || type == 5 ? 4 : type == 1 ? 1 : type == 2 ? 1 : 0;
            if (typeSize == 0 || cnt == 0 || cnt > 0x10000) continue;
            const bool inline4 = typeSize * cnt <= 4;
            const uint64_t vpos = inline4 ? cur + 2 + static_cast<uint64_t>(i) * 12 + 8 : val;
            if (!inline4 && (val < 8 || static_cast<uint64_t>(val) + typeSize * cnt > r.size())) continue;

            auto readVals = [&](std::vector<uint32_t>& dst, size_t maxN) {
                dst.clear();
                for (size_t k = 0; k < cnt && k < maxN; ++k) {
                    if (type == 3) {
                        uint16_t v = 0;
                        if (inline4) {
                            if (k > 1) return;
                            v = little ? static_cast<uint16_t>((val >> (16 * k)) & 0xFFFF)
                                       : static_cast<uint16_t>((val >> (16 * (1 - k))) & 0xFFFF);
                        } else {
                            if (!r.seek(vpos + 2 * k) || !(little ? r.u16le(v) : r.u16be(v))) return;
                        }
                        dst.push_back(v);
                    } else if (type == 4) {
                        if (inline4) {
                            if (k > 0) return;
                            dst.push_back(val);
                        } else {
                            uint32_t v = 0;
                            if (!r.seek(vpos + 4 * k) || !u32(v)) return;
                            dst.push_back(v);
                        }
                    } else {
                        return;
                    }
                }
            };

            if (tag == 0x100 && (type == 3 || type == 4)) {
                std::vector<uint32_t> v;
                readVals(v, 1);
                if (!v.empty()) pw = v[0];
            } else if (tag == 0x101 && (type == 3 || type == 4)) {
                std::vector<uint32_t> v;
                readVals(v, 1);
                if (!v.empty()) ph = v[0];
            } else if (tag == 0x102 && (type == 3 || type == 4)) {
                readVals(pbps, 64);
            } else if (tag == 0x103 && (type == 3 || type == 4)) {
                std::vector<uint32_t> v;
                readVals(v, 1);
                if (!v.empty()) pcomp = v[0];
            } else if (tag == 0x106 && (type == 3 || type == 4)) {
                std::vector<uint32_t> v;
                readVals(v, 1);
                if (!v.empty()) pphoto = v[0];
            } else if (tag == 0x115 && (type == 3 || type == 4)) {
                std::vector<uint32_t> v;
                readVals(v, 1);
                if (!v.empty()) pspp = v[0];
            } else if (tag == 0x11A && type == 5) {
                if (val >= 8 && static_cast<uint64_t>(val) + 8 <= r.size() && r.seek(val)) {
                    uint32_t num = 0, den = 0;
                    if (u32(num) && u32(den) && den != 0) pxres = static_cast<double>(num) / den;
                }
            } else if (tag == 0x11B && type == 5) {
                if (val >= 8 && static_cast<uint64_t>(val) + 8 <= r.size() && r.seek(val)) {
                    uint32_t num = 0, den = 0;
                    if (u32(num) && u32(den) && den != 0) pyres = static_cast<double>(num) / den;
                }
            } else if (tag == 0x128 && type == 3) {
                const uint64_t upos = inline4 ? cur + 2 + static_cast<uint64_t>(i) * 12 + 8 : val;
                uint16_t u = 0;
                if (r.seek(upos) && u16(u)) punit = u;
            }
        }

        if (pw > 0 && ph > 0) {
            ++ifds;
            if (w == 0) {
                w = pw;
                h = ph;
                bps = pbps;
                spp = pspp ? pspp : 1;
                comp = pcomp;
                photo = pphoto;
                xres = pxres;
                yres = pyres;
                unit = punit;
            }
        }

        uint32_t next = 0;
        if (!r.seek(cur + 2 + static_cast<uint64_t>(count) * 12) || !u32(next)) {
            out.status = "повреждён: обрезана ссылка на следующий IFD";
            return false;
        }
        cur = next;
    }

    if (w == 0 || h == 0) {
        out.status = "повреждён: нет тегов ImageWidth/ImageLength";
        return false;
    }

    out.width = w;
    out.height = h;
    if (bps.empty()) bps.push_back(1);
    uint32_t bppSum = 0;
    for (uint32_t b : bps) bppSum += b;
    const bool palette = photo == 3;
    out.bitsPerPixel = palette ? bps[0] : bppSum;
    out.bitDepthText = "SamplesPerPixel=" + std::to_string(spp) + ", BitsPerSample=";
    for (size_t i = 0; i < bps.size() && i < 8; ++i) {
        if (i) out.bitDepthText += ",";
        out.bitDepthText += std::to_string(bps[i]);
    }
    out.bitDepthText += palette ? " (индекс палитры)" : " => " + std::to_string(out.bitsPerPixel) + " бит/пиксель";
    out.compressionCode = comp;
    out.compression = compressionName(comp);

    if ((xres > 0 || yres > 0) && unit != 1) {
        out.hasResolution = true;
        const double k = unit == 3 ? 2.54 : 1.0;
        out.dpiX = xres * k;
        out.dpiY = (yres > 0 ? yres : xres) * k;
    }

    out.extra = "Порядок байтов: ";
    out.extra += little ? "little-endian (II)" : "big-endian (MM)";
    out.extra += "\nФотометрическая интерпретация: ";
    out.extra += photometricName(photo);
    out.extra += "\nСтраниц (IFD с изображением): ";
    out.extra += std::to_string(ifds);
    if (palette) {
        out.extra += "\nПалитра: тег ColorMap (3 × SHORT на цвет, 2^BitsPerSample записей)";
    }
    if (unit == 1 && (xres > 0 || yres > 0)) {
        out.extra += "\nРазрешение задано без единицы измерения (ResolutionUnit = 1)";
    }
    out.ok = true;
    out.status = "OK";
    return true;
}

}
