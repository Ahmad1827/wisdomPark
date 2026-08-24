#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace StudioCore {

class SourceTexture;

class ImageLoader {
public:
    ImageLoader() = delete; // Static processing class

    // Loads an image file from disk and returns an immutable SourceTexture.
    // Forces RGBA (4 channels, 8 bits per channel).
    static std::shared_ptr<SourceTexture> LoadFromFile(const std::string& filePath, std::string& outErrorMessage);

    // Pre-processing: Removes a specific solid color and anything within Euclidean tolerance
    static std::shared_ptr<SourceTexture> ChromaKey(const SourceTexture& source, uint8_t r, uint8_t g, uint8_t b, float tolerance = 15.0f);

    // Pre-processing: Auto-detects and removes fake PNG checkerboards (and JPG artifacts)
    // Samples the top-left corner to find the alternating background colors.
    static std::shared_ptr<SourceTexture> RemoveFakeCheckerboard(const SourceTexture& source, float tolerance = 35.0f);
};

}