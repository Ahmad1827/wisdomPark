#include "Processing/ImageLoader.h"
#include "DataModels/SourceTexture.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <queue>

namespace StudioCore {

std::shared_ptr<SourceTexture> ImageLoader::LoadFromFile(const std::string& filePath, std::string& outErrorMessage) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* rawPixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!rawPixels) {
        outErrorMessage = std::string("Failed to load image from path '") + filePath + "': " + stbi_failure_reason();
        return nullptr;
    }

    if (width <= 0 || height <= 0) {
        outErrorMessage = "Invalid image dimensions in path: " + filePath;
        stbi_image_free(rawPixels);
        return nullptr;
    }

    size_t pixelBufferSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    std::vector<uint8_t> pixelData(rawPixels, rawPixels + pixelBufferSize);

    stbi_image_free(rawPixels);
    outErrorMessage.clear();
    return std::make_shared<SourceTexture>(width, height, std::move(pixelData));
}

std::shared_ptr<SourceTexture> ImageLoader::ChromaKey(const SourceTexture& source, uint8_t r, uint8_t g, uint8_t b, float tolerance) {
    int width = source.GetWidth();
    int height = source.GetHeight();
    const auto& origPixels = source.GetPixels();
    std::vector<uint8_t> newPixels = origPixels;

    for (int i = 0; i < width * height; ++i) {
        int idx = i * 4;
        float dist = std::sqrt(std::pow(newPixels[idx] - r, 2) + 
                               std::pow(newPixels[idx+1] - g, 2) + 
                               std::pow(newPixels[idx+2] - b, 2));

        if (dist <= tolerance) {
            newPixels[idx] = 0;
            newPixels[idx+1] = 0;
            newPixels[idx+2] = 0;
            newPixels[idx + 3] = 0; 
        }
    }

    return std::make_shared<SourceTexture>(width, height, std::move(newPixels));
}

std::shared_ptr<SourceTexture> ImageLoader::RemoveFakeCheckerboard(const SourceTexture& source, float tolerance) {
    int width = source.GetWidth();
    int height = source.GetHeight();
    const auto& origPixels = source.GetPixels();
    std::vector<uint8_t> newPixels = origPixels;

    if (width < 2 || height < 2) return std::make_shared<SourceTexture>(width, height, std::move(newPixels));

    struct ColorRGB { float r, g, b; };
    std::vector<ColorRGB> bgPalette;

    auto samplePixel = [&](int x, int y) {
        int idx = (y * width + x) * 4;
        if (origPixels[idx + 3] == 0) return;

        float pr = origPixels[idx];
        float pg = origPixels[idx + 1];
        float pb = origPixels[idx + 2];

        bool matched = false;
        for (auto& bg : bgPalette) {
            float dr = pr - bg.r;
            float dg = pg - bg.g;
            float db = pb - bg.b;
            if (std::sqrt(dr * dr + dg * dg + db * db) < 12.0f) {
                bg.r = (bg.r + pr) * 0.5f;
                bg.g = (bg.g + pg) * 0.5f;
                bg.b = (bg.b + pb) * 0.5f;
                matched = true;
                break;
            }
        }
        if (!matched && bgPalette.size() < 32) {
            bgPalette.push_back({pr, pg, pb});
        }
    };

    for (int x = 0; x < width; x += std::max(1, width / 32)) {
        samplePixel(x, 0);
        samplePixel(x, height - 1);
    }
    for (int y = 0; y < height; y += std::max(1, height / 32)) {
        samplePixel(0, y);
        samplePixel(width - 1, y);
    }

    if (bgPalette.empty()) {
        return std::make_shared<SourceTexture>(width, height, std::move(newPixels));
    }

    auto isMatchingBg = [&](uint8_t r, uint8_t g, uint8_t b, uint8_t a, float customTol) -> bool {
        if (a == 0) return true;
        for (const auto& bg : bgPalette) {
            float dr = r - bg.r;
            float dg = g - bg.g;
            float db = b - bg.b;
            if (std::sqrt(dr * dr + dg * dg + db * db) <= customTol) return true;
        }
        return false;
    };

    for (int i = 0; i < width * height; ++i) {
        int pIdx = i * 4;
        if (newPixels[pIdx + 3] > 0) {
            if (isMatchingBg(newPixels[pIdx], newPixels[pIdx + 1], newPixels[pIdx + 2], newPixels[pIdx + 3], 18.0f)) {
                newPixels[pIdx] = 0;
                newPixels[pIdx + 1] = 0;
                newPixels[pIdx + 2] = 0;
                newPixels[pIdx + 3] = 0;
            }
        }
    }

    std::vector<bool> despeckleVisited(width * height, false);
    const int despeckleDx[] = {1, -1, 0, 0, 1, 1, -1, -1};
    const int despeckleDy[] = {0, 0, 1, -1, -1, 1, -1, 1};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int startIdx = y * width + x;
            if (newPixels[startIdx * 4 + 3] > 0 && !despeckleVisited[startIdx]) {
                std::vector<int> componentIndices;
                std::queue<std::pair<int, int>> compQueue;

                compQueue.push({x, y});
                despeckleVisited[startIdx] = true;
                componentIndices.push_back(startIdx);

                while (!compQueue.empty()) {
                    auto [cx, cy] = compQueue.front();
                    compQueue.pop();

                    if (componentIndices.size() > 8) break;

                    for (int i = 0; i < 8; ++i) {
                        int nx = cx + despeckleDx[i];
                        int ny = cy + despeckleDy[i];
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int nIdx = ny * width + nx;
                            if (newPixels[nIdx * 4 + 3] > 0 && !despeckleVisited[nIdx]) {
                                despeckleVisited[nIdx] = true;
                                compQueue.push({nx, ny});
                                componentIndices.push_back(nIdx);
                            }
                        }
                    }
                }

                if (componentIndices.size() <= 8) {
                    for (int idx : componentIndices) {
                        newPixels[idx * 4] = 0;
                        newPixels[idx * 4 + 1] = 0;
                        newPixels[idx * 4 + 2] = 0;
                        newPixels[idx * 4 + 3] = 0;
                    }
                }
            }
        }
    }

    return std::make_shared<SourceTexture>(width, height, std::move(newPixels));
}

}