#pragma once
#include <string>
#include <vector>
#include <memory>

namespace StudioCore {

class SpriteDefinition;

struct ProposedAnimation {
    std::string name;
    std::vector<std::string> spriteIds;
    int fps{8};
    bool isLooping{true};
    bool reverseOrder{false};
};

class AnimationBuilder {
public:
    AnimationBuilder() = default;

    // Detects rows of sprites by analyzing their Y coordinates and bounding boxes
    std::vector<ProposedAnimation> DetectByRows(const std::vector<std::shared_ptr<SpriteDefinition>>& sprites);
    
    // Future expansion: DetectByColumns, DetectGrid, etc.
};

}