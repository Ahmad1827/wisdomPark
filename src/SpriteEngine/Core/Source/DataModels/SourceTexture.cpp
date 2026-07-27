#include "DataModels/SourceTexture.h"
#include <utility>

namespace StudioCore {

SourceTexture::SourceTexture(int width, int height, std::vector<uint8_t> pixels)
    : m_width(width), m_height(height), m_pixels(std::move(pixels)) {}

int SourceTexture::GetWidth() const { 
    return m_width; 
}

int SourceTexture::GetHeight() const { 
    return m_height; 
}

const std::vector<uint8_t>& SourceTexture::GetPixels() const { 
    return m_pixels; 
}

bool SourceTexture::IsValid() const { 
    return m_width > 0 && m_height > 0 && !m_pixels.empty(); 
}

}