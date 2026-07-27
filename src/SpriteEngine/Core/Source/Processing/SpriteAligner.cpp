#include "Processing/SpriteAligner.h"
#include "DataModels/AnimationGroup.h"
#include "DataModels/Project.h"

namespace StudioCore {

AlignmentResult SpriteAligner::ComputeAlignment(const SpriteDefinition& sprite) {
    AlignmentResult res;
    const auto& rect = sprite.GetSourceRect();
    const auto& center = sprite.GetCenter();

    // Pivot X: Center of Mass relative to the local bounding box
    if (rect.width > 0) {
        res.alignedPivot.x = (center.x - rect.x) / static_cast<float>(rect.width);
    } else {
        res.alignedPivot.x = 0.5f;
    }

    // Pivot Y / Baseline: Anchor feet to the bottom edge of the sprite's tight bounding box
    res.alignedPivot.y = 1.0f;
    res.alignedBaseline = 0.0f;

    // Render Offset: Pixel shift required if rendering to a unified canvas (Future Export)
    res.renderOffset.x = rect.width * res.alignedPivot.x;
    res.renderOffset.y = rect.height - res.alignedBaseline;

    return res;
}

std::map<std::string, AlignmentResult> SpriteAligner::ComputeAnimationAlignment(const AnimationGroup& anim, const Project& project) {
    std::map<std::string, AlignmentResult> results;
    for (const auto& id : anim.GetFrames()) {
        auto sprite = project.GetSpriteById(id);
        if (sprite) {
            results[id] = ComputeAlignment(*sprite);
        }
    }
    return results;
}

}