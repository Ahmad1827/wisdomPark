#include "Commands/RepackFramesCommand.h"
#include "DataModels/Project.h"
#include "DataModels/SourceTexture.h"
#include "DataModels/SpriteDefinition.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace StudioCore {

RepackFramesCommand::RepackFramesCommand(std::shared_ptr<Project> project)
    : m_project(project) {}

void RepackFramesCommand::Execute() {
    if (!m_newTexture) {
        m_oldTexture = m_project->GetTexture();
        m_oldSprites = m_project->GetSprites();

        if (m_oldSprites.empty() || !m_oldTexture) return;

        std::vector<Rect> rawRects;
        for (const auto& s : m_oldSprites) {
            rawRects.push_back(s->GetSourceRect());
        }

        std::vector<Rect> mainSprites;
        std::vector<Rect> fragments;
        for (const auto& r : rawRects) {
            if (r.width > 35 && r.height > 35) {
                mainSprites.push_back(r);
            } else {
                fragments.push_back(r);
            }
        }

        for (const auto& frag : fragments) {
            if (mainSprites.empty()) break;
            float minD = 999999.0f;
            int bestIdx = -1;
            float fcx = frag.x + frag.width / 2.0f;
            float fcy = frag.y + frag.height / 2.0f;
            for (size_t i = 0; i < mainSprites.size(); ++i) {
                float mcx = mainSprites[i].x + mainSprites[i].width / 2.0f;
                float mcy = mainSprites[i].y + mainSprites[i].height / 2.0f;
                float d = (fcx - mcx) * (fcx - mcx) + (fcy - mcy) * (fcy - mcy);
                if (d < minD) {
                    minD = d;
                    bestIdx = i;
                }
            }
            if (bestIdx != -1) {
                float minX = std::min(mainSprites[bestIdx].x, frag.x);
                float minY = std::min(mainSprites[bestIdx].y, frag.y);
                float maxX = std::max(mainSprites[bestIdx].x + mainSprites[bestIdx].width, frag.x + frag.width);
                float maxY = std::max(mainSprites[bestIdx].y + mainSprites[bestIdx].height, frag.y + frag.height);
                mainSprites[bestIdx].x = minX;
                mainSprites[bestIdx].y = minY;
                mainSprites[bestIdx].width = maxX - minX;
                mainSprites[bestIdx].height = maxY - minY;
            }
        }

        std::sort(mainSprites.begin(), mainSprites.end(), [](const Rect& a, const Rect& b) {
            return a.x < b.x; 
        });

        float maxWidth = 0.0f;
        float maxHeight = 0.0f;
        for (const auto& r : mainSprites) {
            if (r.width > maxWidth) maxWidth = r.width;
            if (r.height > maxHeight) maxHeight = r.height;
        }

        int newWidth = static_cast<int>(maxWidth * mainSprites.size());
        int newHeight = static_cast<int>(maxHeight);
        std::vector<uint8_t> newPixels(newWidth * newHeight * 4, 0); 

        int w = m_oldTexture->GetWidth();
        const auto& pixels = m_oldTexture->GetPixels();

        m_newSprites.clear();
        for (size_t i = 0; i < mainSprites.size(); ++i) {
            const Rect& rect = mainSprites[i];
            
            int destX = static_cast<int>(i * maxWidth + (maxWidth - rect.width) / 2.0f);
            int destY = static_cast<int>(maxHeight - rect.height); 

            for (int y = 0; y < static_cast<int>(rect.height); ++y) {
                for (int x = 0; x < static_cast<int>(rect.width); ++x) {
                    int oldIdx = ((static_cast<int>(rect.y) + y) * w + (static_cast<int>(rect.x) + x)) * 4;
                    int newIdx = ((destY + y) * newWidth + (destX + x)) * 4;
                    newPixels[newIdx]   = pixels[oldIdx];
                    newPixels[newIdx+1] = pixels[oldIdx+1];
                    newPixels[newIdx+2] = pixels[oldIdx+2];
                    newPixels[newIdx+3] = pixels[oldIdx+3];
                }
            }
            
            Rect newRect{ static_cast<float>(i * maxWidth), 0.0f, maxWidth, maxHeight };
            auto def = std::make_shared<SpriteDefinition>("frame_" + std::to_string(i), newRect);
            m_newSprites.push_back(def);
        }

        m_newTexture = std::make_shared<SourceTexture>(newWidth, newHeight, std::move(newPixels));
    }

    m_project->SetTexture(std::const_pointer_cast<SourceTexture>(m_newTexture));
    m_project->SetSprites(m_newSprites); 
}
void RepackFramesCommand::Undo() {
    if (m_project && m_oldTexture) {
        m_project->SetTexture(std::const_pointer_cast<SourceTexture>(m_oldTexture));
        m_project->SetSprites(m_oldSprites);
    }
}

}