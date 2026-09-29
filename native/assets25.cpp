#include "assets25.h"
#include <algorithm>
#include <stdexcept>
#include <string>
#include "palette25.h"

namespace stoneage {
namespace {
[[noreturn]] void invalid(const char* message) { throw std::runtime_error(message); }
std::uint32_t u32(const Bytes& b, std::size_t p) {
    if (p > b.size() || b.size() - p < 4) invalid("Truncated 32-bit asset field");
    return std::uint32_t(b[p]) | (std::uint32_t(b[p+1]) << 8) |
        (std::uint32_t(b[p+2]) << 16) | (std::uint32_t(b[p+3]) << 24);
}
std::uint16_t u16(const Bytes& b, std::size_t p) {
    if (p > b.size() || b.size() - p < 2) invalid("Truncated 16-bit asset field");
    return b[p] | (std::uint16_t(b[p+1]) << 8);
}
Bytes read(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("Cannot open " + path.string());
    auto size = f.tellg();
    if (size < 0 || size > 64*1024*1024) invalid("Invalid asset index size");
    Bytes b(static_cast<std::size_t>(size));
    f.seekg(0);
    if (!f.read(reinterpret_cast<char*>(b.data()), b.size())) invalid("Short asset read");
    return b;
}
}
Image25 decodeRD(const Bytes& b) {
    if (b.size() < 16 || b[0] != 'R' || b[1] != 'D') invalid("Missing RD image header");
    Image25 image{u32(b, 4), u32(b, 8), 0, 0, {}};
    auto size = u32(b, 12);
    auto count = std::uint64_t(image.width) * image.height;
    if (!count || image.width > 8192 || image.height > 8192 || count > 16*1024*1024)
        invalid("Invalid RD dimensions");
    image.pixels.reserve(static_cast<std::size_t>(count));
    if (b[2] == 0) {
        // Legacy encoder writes a pointer-derived value to RD.size on raw images;
        // the original decoder ignores it. Bound this path by the ADRN record.
        if (b.size() - 16 != count) invalid("Wrong uncompressed RD size");
        image.pixels.assign(b.begin() + 16, b.end());
        return image;
    }
    if (size < 16 || size > b.size()) invalid("RD payload outside record");
    if (b[2] != 1) invalid("Unsupported RD format (2.5 expects indexed 0/1)");
    std::size_t p = 16;
    auto byte = [&]() -> std::uint8_t {
        if (p >= size) invalid("Truncated RD run");
        return b[p++];
    };
    while (p < size) {
        auto tag = byte();
        bool repeat = (tag & 0x80) != 0;
        auto value = repeat && !(tag & 0x40) ? byte() : 0;
        std::size_t length = tag & 15;
        if (repeat && (tag & 0x20)) {
            auto hi = byte(); auto lo = byte();
            length = (length << 16) | (std::size_t(hi) << 8) | lo;
        } else if (tag & 0x10) length = (length << 8) | byte();
        if (!length || length > count - image.pixels.size()) invalid("RD run exceeds image");
        if (repeat) image.pixels.insert(image.pixels.end(), length, value);
        else {
            if (length > size - p) invalid("Truncated RD literal");
            image.pixels.insert(image.pixels.end(), b.begin() + p, b.begin() + p + length);
            p += length;
        }
    }
    if (image.pixels.size() != count) invalid("Incomplete RD image");
    return image;
}
Assets25::Assets25(const std::filesystem::path& root) {
    auto data = root / "data";
    auto index = read(data / "adrn_15.bin");
    if (index.empty() || index.size() % 80) invalid("Invalid 2.5 graphics index");
    real_.open(data / "real_15.bin", std::ios::binary | std::ios::ate);
    if (!real_) invalid("Cannot open data/real_15.bin");
    auto realSize = real_.tellg();
    if (realSize < 0) invalid("Cannot size real_15.bin");
    for (std::size_t p = 0; p < index.size(); p += 80) {
        Graphic25 g{u32(index,p+4),u32(index,p+8),u32(index,p+20),u32(index,p+24),
                    static_cast<std::int32_t>(u32(index,p+12)),static_cast<std::int32_t>(u32(index,p+16))};
        if (g.size < 16 || g.size > 64*1024*1024 || std::uint64_t(g.offset)+g.size > std::uint64_t(realSize))
            invalid("Graphics record outside real_15.bin");
        // Original loadrealbin.cpp also uses last-record-wins for repeated IDs.
        graphics_[u32(index,p)] = g;
    }
    auto spriteIndex = read(data / "spradrn_5.bin");
    auto spriteData = read(data / "spr_4.bin");
    if (spriteIndex.empty() || spriteIndex.size() % 12) invalid("Invalid 2.5 sprite index");
    for (std::size_t p = 0; p < spriteIndex.size(); p += 12) {
        std::size_t offset = u32(spriteIndex,p+4);
        auto& animations = sprites_[u32(spriteIndex,p)];
        animations.clear(); // Same last-record-wins rule as the original sprite loader.
        for (unsigned a = 0; a < u16(spriteIndex,p+8); ++a) {
            Animation25 animation{u16(spriteData,offset),u16(spriteData,offset+2),u32(spriteData,offset+4),{}};
            auto frames = u32(spriteData,offset+8); offset += 12;
            if (!frames || offset > spriteData.size() || frames > (spriteData.size()-offset)/10)
                invalid("Sprite frames outside spr_4.bin");
            for (unsigned f = 0; f < frames; ++f, offset += 10) {
                Frame25 frame{u32(spriteData,offset),static_cast<std::int16_t>(u16(spriteData,offset+4)),
                              static_cast<std::int16_t>(u16(spriteData,offset+6)),u16(spriteData,offset+8)};
                if (frame.bitmap != UINT32_MAX && !graphics_.count(frame.bitmap)) invalid("Sprite refers to missing bitmap");
                animation.frames.push_back(frame);
            }
            animations.push_back(std::move(animation));
        }
        if (animations.empty()) invalid("Sprite has no animations");
    }
    auto palettePath = data / "pal" / "Palet_1.sap";
    if (!std::filesystem::exists(palettePath)) palettePath = data / "pal" / "palet_1.sap";
    auto palette = read(palettePath);
    if (palette.size() < 224*3) invalid("Truncated SAP palette");
    for (unsigned i = 0; i < 32; ++i) {
        auto slot = i < 16 ? i : i + 224;
        palette_[slot] = {reservedPalette[i][0],reservedPalette[i][1],reservedPalette[i][2],255};
    }
    for (unsigned i = 0; i < 224; ++i)
        palette_[i+16] = {palette[i*3+2],palette[i*3+1],palette[i*3],255};
    palette_[0][3] = 0;
    palette_[168] = {0,0,0,255}; // original InitPalette special black slot
}
Image25 Assets25::image(std::uint32_t bitmap) {
    const auto& g = graphics_.at(bitmap);
    Bytes bytes(g.size);
    real_.clear(); real_.seekg(g.offset);
    if (!real_.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) invalid("Short RD record read");
    Image25 result;
    try { result = decodeRD(bytes); }
    catch (const std::exception& e) { throw std::runtime_error("Bitmap " + std::to_string(bitmap) + ": " + e.what()); }
    if (result.width != g.width || result.height != g.height) invalid("RD/index dimensions differ");
    result.x = g.x; result.y = g.y;
    return result;
}
Bytes Assets25::rgba(const Image25& image) const {
    Bytes result(image.pixels.size()*4);
    for (std::size_t y = 0; y < image.height; ++y) {
        for (std::size_t x = 0; x < image.width; ++x) {
            const auto& color = palette_[image.pixels[(image.height-1-y)*image.width+x]];
            std::copy(color.begin(), color.end(), result.begin()+(y*image.width+x)*4);
        }
    }
    return result;
}
}
