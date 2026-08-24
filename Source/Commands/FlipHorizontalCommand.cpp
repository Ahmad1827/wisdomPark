#include "Commands/FlipHorizontalCommand.h"
#include "DataModels/Project.h"
#include "DataModels/SourceTexture.h"
#include "DataModels/SpriteDefinition.h"
#include <utility>

namespace StudioCore {

FlipHorizontalCommand::FlipHorizontalCommand(std::shared_ptr<Project> project)
    : m_project(project) {}

void FlipHorizontalCommand::Execute() {
    if (!m_newTexture) {
        m_oldTexture = m_project->GetTexture();
        auto sprites = m_project->GetSprites();

        if (sprites.empty() || !m_oldTexture) return;

        int width = m_oldTexture->GetWidth();
        int height = m_oldTexture->GetHeight();
        std::vector<uint8_t> newPixels = m_oldTexture->GetPixels();

        for (const auto& sprite : sprites) {
            Rect rect = sprite->GetSourceRect();
            int startX = static_cast<int>(rect.x);
            int startY = static_cast<int>(rect.y);
            int rectW = static_cast<int>(rect.width);
            int rectH = static_cast<int>(rect.height);

            for (int y = startY; y < startY + rectH; ++y) {
                for (int x = 0; x < rectW / 2; ++x) {
                    int leftIdx = (y * width + (startX + x)) * 4;
                    int rightIdx = (y * width + (startX + rectW - 1 - x)) * 4;

                    std::swap(newPixels[leftIdx], newPixels[rightIdx]);
                    std::swap(newPixels[leftIdx + 1], newPixels[rightIdx + 1]);
                    std::swap(newPixels[leftIdx + 2], newPixels[rightIdx + 2]);
                    std::swap(newPixels[leftIdx + 3], newPixels[rightIdx + 3]);
                }
            }
        }

        m_newTexture = std::make_shared<SourceTexture>(width, height, std::move(newPixels));
    }

    m_project->SetTexture(std::const_pointer_cast<SourceTexture>(m_newTexture));
}

void FlipHorizontalCommand::Undo() {
    if (m_project && m_oldTexture) {
        m_project->SetTexture(std::const_pointer_cast<SourceTexture>(m_oldTexture));
    }
}

}