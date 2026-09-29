#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>

namespace stoneage {
using Bytes = std::vector<std::uint8_t>;
struct Image25 {
    std::uint32_t width, height;
    int x = 0, y = 0;
    Bytes pixels; // palette indices, bottom row first (original RD convention)
};
// Bounded port of system/unpack.cpp's RD decoder; no Win32 struct casts.
Image25 decodeRD(const Bytes& bytes);
struct Frame25 { std::uint32_t bitmap; std::int16_t x, y; std::uint16_t sound; };
struct Animation25 {
    std::uint16_t direction, action;
    std::uint32_t duration;
    std::vector<Frame25> frames;
};
struct Graphic25 {
    std::uint32_t offset, size, width, height;
    std::int32_t x, y;
};
class Assets25 {
public:
    explicit Assets25(const std::filesystem::path& clientRoot);
    Image25 image(std::uint32_t bitmap);
    Bytes rgba(const Image25& image) const;
    const auto& sprites() const { return sprites_; }
    const auto& graphics() const { return graphics_; }
private:
    std::ifstream real_;
    std::map<std::uint32_t, Graphic25> graphics_;
    std::map<std::uint32_t, std::vector<Animation25>> sprites_;
    std::array<std::array<std::uint8_t, 4>, 256> palette_{};
};
}
