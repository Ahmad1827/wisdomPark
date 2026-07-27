#include "Systems/AnimationBuilder.h"
#include "DataModels/SpriteDefinition.h"
#include <algorithm>
#include <cmath>

namespace StudioCore {

std::vector<ProposedAnimation> AnimationBuilder::DetectByRows(const std::vector<std::shared_ptr<SpriteDefinition>>& sprites) {
    std::vector<ProposedAnimation> result;
    if (sprites.empty()) return result;

    // 1. Sort all sprites primarily by their Y coordinate
    auto sortedSprites = sprites;
    std::sort(sortedSprites.begin(), sortedSprites.end(), [](const auto& a, const auto& b) {
        return a->GetSourceRect().y < b->GetSourceRect().y;
    });

    // 2. Group into rows using a height-based threshold
    std::vector<std::vector<std::shared_ptr<SpriteDefinition>>> rows;
    rows.push_back({sortedSprites[0]});

    for (size_t i = 1; i < sortedSprites.size(); ++i) {
        auto& currentRow = rows.back();
        float avgY = 0.0f;
        for (const auto& s : currentRow) {
            avgY += s->GetSourceRect().y;
        }
        avgY /= currentRow.size();

        // If the Y difference is less than 50% of the sprite's height, consider it the same row
        float heightThreshold = sortedSprites[i]->GetSourceRect().height * 0.5f;
        
        if (std::abs(sortedSprites[i]->GetSourceRect().y - avgY) < heightThreshold) {
            currentRow.push_back(sortedSprites[i]);
        } else {
            rows.push_back({sortedSprites[i]});
        }
    }

    // 3. Process each row, sort left-to-right, and create proposed animations
    std::vector<std::string> smartNames = {"Idle", "Walk", "Run", "Jump", "Attack", "Death", "Hit"};
    int rowIndex = 0;

    for (auto& row : rows) {
        // Sort left-to-right
        std::sort(row.begin(), row.end(), [](const auto& a, const auto& b) {
            return a->GetSourceRect().x < b->GetSourceRect().x;
        });

        ProposedAnimation anim;
        anim.name = (rowIndex < smartNames.size()) ? smartNames[rowIndex] : "Action_" + std::to_string(rowIndex + 1);
        
        for (const auto& s : row) {
            anim.spriteIds.push_back(s->GetId());
        }
        
        // Smart defaults based on typical game states
        if (anim.name == "Idle") anim.fps = 6;
        else if (anim.name == "Run") anim.fps = 12;
        else anim.fps = 8;
        
        anim.isLooping = (anim.name != "Death");
        
        result.push_back(anim);
        rowIndex++;
    }

    return result;
}

}