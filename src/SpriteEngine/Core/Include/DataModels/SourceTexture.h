#pragma once
#include <vector>
#include <cstdint>

namespace StudioCore {

class SourceTexture {
public:
    SourceTexture() = default;
    SourceTexture(int width, int height, std::vector<uint8_t> pixels);

    int GetWidth() const;
    int GetHeight() const;
    const std::vector<uint8_t>& GetPixels() const;
    std::vector<uint8_t>& GetPixelsMutable() { return m_pixels; }
    void SetPixels(const std::vector<uint8_t>& pixels) { m_pixels = pixels; }
    bool IsValid() const;

private:
    int m_width{0};
    int m_height{0};
    std::vector<uint8_t> m_pixels;
};

}