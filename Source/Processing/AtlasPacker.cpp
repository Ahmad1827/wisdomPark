#include "Processing/AtlasPacker.h"
#include "DataModels/Project.h"
#include "DataModels/SourceTexture.h"
#include "DataModels/SpriteDefinition.h"
#include "Processing/SpriteAligner.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace StudioCore {

AtlasResult AtlasPacker::PackUniformGrid(const Project& project, int padding) {
    AtlasResult result;
    auto texture = project.GetTexture();
    auto originalSprites = project.GetSprites();

    if (!texture || originalSprites.empty() || texture->GetWidth() == 0 || texture->GetHeight() == 0) {
        return result;
    }

    std::vector<std::shared_ptr<SpriteDefinition>> sortedSprites = originalSprites;
    std::sort(sortedSprites.begin(), sortedSprites.end(), [](const auto& a, const auto& b) {
        const auto& rA = a->GetSourceRect();
        const auto& rB = b->GetSourceRect();
        int centerYA = rA.y + rA.height / 2;
        int centerYB = rB.y + rB.height / 2;
        int threshold = std::max(1, static_cast<int>(std::min(rA.height, rB.height) / 2.0f));
        if (std::abs(centerYA - centerYB) > threshold) {
            return centerYA < centerYB; 
        }
        return rA.x < rB.x; 
    });

    std::vector<std::vector<std::shared_ptr<SpriteDefinition>>> grid;
    grid.push_back({sortedSprites[0]});
    
    for (size_t i = 1; i < sortedSprites.size(); ++i) {
        const auto& currentSprite = sortedSprites[i];
        const auto& lastSprite = grid.back().back();
        const auto& rCurr = currentSprite->GetSourceRect();
        const auto& rLast = lastSprite->GetSourceRect();
        int currCenterY = rCurr.y + rCurr.height / 2;
        int lastCenterY = rLast.y + rLast.height / 2;
        int threshold = std::max(1, static_cast<int>(std::min(rCurr.height, rLast.height) / 2.0f));
        if (std::abs(currCenterY - lastCenterY) > threshold) {
            grid.push_back({currentSprite}); 
        } else {
            grid.back().push_back(currentSprite); 
        }
    }

    float maxLeft = 0.0f;
    float maxRight = 0.0f;
    float maxUp = 0.0f;
    float maxDown = 0.0f;
    size_t maxCols = 0;

    for (const auto& row : grid) {
        maxCols = std::max(maxCols, row.size());
        for (const auto& sprite : row) {
            AlignmentResult align = SpriteAligner::ComputeAlignment(*sprite);
            const auto& rect = sprite->GetSourceRect();
            float left = rect.width * align.alignedPivot.x;
            float right = rect.width - left;
            float up = rect.height - align.alignedBaseline;
            float down = align.alignedBaseline;
            maxLeft = std::max(maxLeft, left);
            maxRight = std::max(maxRight, right);
            maxUp = std::max(maxUp, up);
            maxDown = std::max(maxDown, down);
        }
    }

    int safePadding = std::max(0, padding);
    int cellWidth = static_cast<int>(std::ceil(maxLeft) + std::ceil(maxRight)) + safePadding * 2;
    int cellHeight = static_cast<int>(std::ceil(maxUp) + std::ceil(maxDown)) + safePadding * 2;

    int cellPivotX = safePadding + static_cast<int>(std::ceil(maxLeft));
    int cellPivotY = safePadding + static_cast<int>(std::ceil(maxUp));

    int rows = static_cast<int>(grid.size());
    int cols = static_cast<int>(maxCols);

    result.image.create(cols * cellWidth, rows * cellHeight, sf::Color::Transparent);
    sf::Image sourceImg;
    sourceImg.create(texture->GetWidth(), texture->GetHeight(), texture->GetPixels().data());

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < static_cast<int>(grid[r].size()); ++c) {
            const auto& sprite = grid[r][c];
            AlignmentResult align = SpriteAligner::ComputeAlignment(*sprite);
            const auto& rect = sprite->GetSourceRect();
            float left = rect.width * align.alignedPivot.x;
            float up = rect.height - align.alignedBaseline;
            int destX = c * cellWidth + cellPivotX - static_cast<int>(std::round(left));
            int destY = r * cellHeight + cellPivotY - static_cast<int>(std::round(up));

            result.image.copy(sourceImg, destX, destY, sf::IntRect(rect.x, rect.y, rect.width, rect.height), false);

            PackedSprite ps;
            ps.id = sprite->GetId();
            ps.name = ps.id;
            ps.x = destX;
            ps.y = destY;
            ps.width = rect.width;
            ps.height = rect.height;
            ps.pivotX = align.alignedPivot.x;
            ps.pivotY = align.alignedPivot.y;
            ps.baseline = align.alignedBaseline;
            result.sprites.push_back(ps);
        }
    }

    return result;
}

}