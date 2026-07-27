#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>

namespace StudioCore {

class SourceTexture;
class SpriteDefinition;

enum class DetectionMode {
    AlphaThreshold,
    SolidColor,
    MagicWand
};

struct DetectionConfig {
    DetectionMode mode{DetectionMode::AlphaThreshold};
    uint8_t alphaThreshold{10};
    uint8_t targetColor[3]{255, 0, 255};
    float colorTolerance{5.0f};
    int minSpriteSize{4};
    int maxSpriteSize{4096};
    bool use8WayConnectivity{true};
    int padding{0};
};

class SpriteDetector {
public:
    SpriteDetector() = delete;

    static std::vector<std::shared_ptr<SpriteDefinition>> Detect(
        const SourceTexture& texture,
        const DetectionConfig& config,
        std::atomic<float>& progress,
        std::atomic<bool>& cancelToken
    );
};

}