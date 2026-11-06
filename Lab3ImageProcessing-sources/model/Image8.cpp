#include "Image8.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace imgproc {

Image8::Image8(int w, int h)
    : width(w), height(h), rgb(static_cast<size_t>(w > 0 ? w : 0) * (h > 0 ? h : 0) * 3) {}

uint8_t Image8::grayAt(int x, int y) const {
    const uint8_t* p = row(y) + static_cast<size_t>(x) * 3;
    return luminance(p[0], p[1], p[2]);
}

namespace {

bool skipWhitespaceAndComments(std::FILE* f) {
    int c = std::fgetc(f);
    for (;;) {
        if (c == '#') {
            while (c != '\n' && c != EOF) c = std::fgetc(f);
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            c = std::fgetc(f);
        } else {
            break;
        }
    }
    if (c == EOF) return false;
    std::ungetc(c, f);
    return true;
}

bool readToken(std::FILE* f, std::string& tok) {
    if (!skipWhitespaceAndComments(f)) return false;
    tok.clear();
    int c;
    while ((c = std::fgetc(f)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '#') {
            if (c == '#') std::ungetc(c, f);
            break;
        }
        tok.push_back(static_cast<char>(c));
    }
    return !tok.empty();
}

bool readInt(std::FILE* f, int& v) {
    std::string tok;
    if (!readToken(f, tok)) return false;
    char* end = nullptr;
    const long val = std::strtol(tok.c_str(), &end, 10);
    if (end == tok.c_str() || *end != '\0' || val < 0 || val > 2147483647L) return false;
    v = static_cast<int>(val);
    return true;
}

}

bool Image8::loadPPM(const std::string& path, std::string* err) {
    auto fail = [&](const char* msg) {
        if (err) *err = msg;
        return false;
    };
    std::FILE* f = nullptr;
#ifdef _WIN32
    {
        const int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        std::wstring w(static_cast<size_t>(n > 0 ? n : 1), L'\0');
        if (n > 0) MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, w.data(), n);
        f = _wfopen(w.c_str(), L"rb");
    }
#else
    f = std::fopen(path.c_str(), "rb");
#endif
    if (!f) return fail("cannot open file");
    char magic[3] = {};
    if (std::fread(magic, 1, 2, f) != 2 || (std::strcmp(magic, "P5") != 0 && std::strcmp(magic, "P6") != 0)) {
        std::fclose(f);
        return fail("not a P5/P6 PPM file");
    }
    int w = 0, h = 0, maxval = 0;
    if (!readInt(f, w) || !readInt(f, h) || !readInt(f, maxval) || w <= 0 || h <= 0 || maxval <= 0 || maxval > 255) {
        std::fclose(f);
        return fail("bad PPM header");
    }
    const bool gray = magic[1] == '5';
    width = w;
    height = h;
    rgb.assign(pixelCount() * 3, 0);
    std::vector<uint8_t> plane(static_cast<size_t>(w) * h);
    if (gray) {
        if (std::fread(plane.data(), 1, plane.size(), f) != plane.size()) {
            std::fclose(f);
            return fail("truncated PPM data");
        }
        for (size_t i = 0; i < plane.size(); ++i) {
            rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = plane[i];
        }
    } else {
        if (std::fread(rgb.data(), 1, rgb.size(), f) != rgb.size()) {
            std::fclose(f);
            return fail("truncated PPM data");
        }
    }
    std::fclose(f);
    if (err) err->clear();
    return true;
}

bool Image8::savePPM(const std::string& path, std::string* err) const {
    auto fail = [&](const char* msg) {
        if (err) *err = msg;
        return false;
    };
    if (empty()) return fail("empty image");
    std::FILE* f = nullptr;
#ifdef _WIN32
    {
        const int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        std::wstring w(static_cast<size_t>(n > 0 ? n : 1), L'\0');
        if (n > 0) MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, w.data(), n);
        f = _wfopen(w.c_str(), L"wb");
    }
#else
    f = std::fopen(path.c_str(), "wb");
#endif
    if (!f) return fail("cannot open file for writing");
    std::fprintf(f, "P6\n%d %d\n255\n", width, height);
    const bool ok = std::fwrite(rgb.data(), 1, rgb.size(), f) == rgb.size();
    std::fclose(f);
    if (!ok) return fail("write failed");
    if (err) err->clear();
    return true;
}

}
