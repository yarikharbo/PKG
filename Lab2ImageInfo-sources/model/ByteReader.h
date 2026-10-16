#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace img {

class ByteReader {
public:
    ~ByteReader();

    bool open(const std::string& path);
    void close();
    bool isOpen() const { return f_ != nullptr; }
    uint64_t size() const { return size_; }

    bool seek(uint64_t pos);
    uint64_t tell() const;
    bool read(uint8_t* dst, size_t n);
    bool skip(uint64_t n);

    bool u8(uint8_t& v);
    bool u16le(uint16_t& v);
    bool u16be(uint16_t& v);
    bool u32le(uint32_t& v);
    bool u32be(uint32_t& v);
    bool s32le(int32_t& v);

    static std::string extensionOf(const std::string& path);

private:
    std::FILE* f_ = nullptr;
    uint64_t size_ = 0;
};

}
