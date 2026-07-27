#pragma once
#include <string>
#include <memory>

namespace StudioCore {

class SourceTexture;

class ImageLoader {
public:
    ImageLoader() = delete; // Static processing class

    // Loads an image file from disk and returns an immutable SourceTexture.
    // Forces RGBA (4 channels, 8 bits per channel).
    static std::shared_ptr<SourceTexture> LoadFromFile(const std::string& filePath, std::string& outErrorMessage);
};

}