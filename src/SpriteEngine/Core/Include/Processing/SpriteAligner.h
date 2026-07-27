#pragma once
#include <string>
#include <map>
#include "DataModels/SpriteDefinition.h"

namespace StudioCore {

class AnimationGroup;
class Project;

struct AlignmentResult {
    Point alignedPivot{0.5f, 1.0f};
    float alignedBaseline{0.0f};
    Point renderOffset{0.0f, 0.0f};
};

class SpriteAligner {
public:
    // Computes the ideal alignment for a single sprite using its Center of Mass and Bounds
    static AlignmentResult ComputeAlignment(const SpriteDefinition& sprite);
    
    // Future-proof: Computes all offsets for an animation group (used later by the Exporter)
    static std::map<std::string, AlignmentResult> ComputeAnimationAlignment(const AnimationGroup& anim, const Project& project);
};

}