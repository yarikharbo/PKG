#include "ByteReader.h"

#include <algorithm>
#include <cctype>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace img {

namespace {

#ifdef _WIN32
int seek64(std::FILE* f, long long off, int whence) { return _fseeki64(f, off, whence); }
long long tell64(std::FILE* f) { return _ftelli64(f); }
#else
int seek64(std::FILE* f, long long off, int whence) { return fseeko(f, off, whence); }
long long tell64(std::FILE* f) { return ftello(f); }
#endif

}

ByteReader::~ByteReader() { close(); }

#ifdef _WIN32

std::FILE* openFile(const std::string& path) {
    const int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (n <= 0) return nullptr;
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, w.data(), n);
    return _wfopen(w.c_str(), L"rb");
}

#else

std::FILE* openFile(const std::string& path) { return std::fopen(path.c_str(), "rb"); }

#endif

bool ByteReader::open(const std::string& path) {
    close();
    std::FILE* f = openFile(path);
    if (!f) return false;
    f_ = f;
    if (seek64(f_, 0, SEEK_END) != 0) { close(); return false; }
    const long long sz = tell64(f_);
    if (sz < 0) { close(); return false; }
    size_ = static_cast<uint64_t>(sz);
    if (seek64(f_, 0, SEEK_SET) != 0) { close(); return false; }
    return true;
}

void ByteReader::close() {
    if (f_) {
        std::fclose(f_);
        f_ = nullptr;
    }
    size_ = 0;
}

bool ByteReader::seek(uint64_t pos) {
    if (!f_ || pos > static_cast<uint64_t>(INT64_MAX)) return false;
    return seek64(f_, static_cast<long long>(pos), SEEK_SET) == 0;
}

uint64_t ByteReader::tell() const {
    if (!f_) return 0;
    const long long p = tell64(f_);
    return p < 0 ? UINT64_MAX : static_cast<uint64_t>(p);
}

bool ByteReader::read(uint8_t* dst, size_t n) {
    if (!f_) return false;
    return n == 0 || std::fread(dst, 1, n, f_) == n;
}

bool ByteReader::skip(uint64_t n) {
    if (!f_) return false;
    if (n > static_cast<uint64_t>(INT64_MAX)) return false;
    return seek64(f_, static_cast<long long>(n), SEEK_CUR) == 0;
}

bool ByteReader::u8(uint8_t& v) { return read(&v, 1); }

bool ByteReader::u16le(uint16_t& v) {
    uint8_t b[2];
    if (!read(b, 2)) return false;
    v = static_cast<uint16_t>(b[0] | (b[1] << 8));
    return true;
}

bool ByteReader::u16be(uint16_t& v) {
    uint8_t b[2];
    if (!read(b, 2)) return false;
    v = static_cast<uint16_t>((b[0] << 8) | b[1]);
    return true;
}

bool ByteReader::u32le(uint32_t& v) {
    uint8_t b[4];
    if (!read(b, 4)) return false;
    v = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
        (static_cast<uint32_t>(b[2]) << 16) | (static_cast<uint32_t>(b[3]) << 24);
    return true;
}

bool ByteReader::u32be(uint32_t& v) {
    uint8_t b[4];
    if (!read(b, 4)) return false;
    v = (static_cast<uint32_t>(b[0]) << 24) | (static_cast<uint32_t>(b[1]) << 16) |
        (static_cast<uint32_t>(b[2]) << 8) | static_cast<uint32_t>(b[3]);
    return true;
}

bool ByteReader::s32le(int32_t& v) {
    uint32_t u;
    if (!u32le(u)) return false;
    v = static_cast<int32_t>(u);
    return true;
}

std::string ByteReader::extensionOf(const std::string& path) {
    const size_t dot = path.find_last_of('.');
    const size_t slash = path.find_last_of("/\\");
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return {};
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

}
