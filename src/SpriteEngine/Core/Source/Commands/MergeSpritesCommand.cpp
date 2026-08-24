#include "Commands/MergeSpritesCommand.h"
#include "DataModels/Project.h"
#include "DataModels/SpriteDefinition.h"
#include <algorithm>

namespace StudioCore {

MergeSpritesCommand::MergeSpritesCommand(std::shared_ptr<Project> project) 
    : m_project(project) {}

void MergeSpritesCommand::Execute() {
    if (!m_executed) {
        m_oldSprites = m_project->GetSprites();
        if (m_oldSprites.empty()) return;

        std::vector<Rect> rects;
        for (const auto& s : m_oldSprites) {
            rects.push_back(s->GetSourceRect());
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t i = 0; i < rects.size(); ++i) {
                for (size_t j = i + 1; j < rects.size(); ++j) {
                    Rect& r1 = rects[i];
                    Rect& r2 = rects[j];

                    bool intersect = (r1.x <= r2.x + r2.width && 
                                      r1.x + r1.width >= r2.x &&
                                      r1.y <= r2.y + r2.height && 
                                      r1.y + r1.height >= r2.y);
                    
                    if (intersect) {
                        float minX = std::min(r1.x, r2.x);
                        float minY = std::min(r1.y, r2.y);
                        float maxX = std::max(r1.x + r1.width, r2.x + r2.width);
                        float maxY = std::max(r1.y + r1.height, r2.y + r2.height);
                        
                        r1.x = minX;
                        r1.y = minY;
                        r1.width = maxX - minX;
                        r1.height = maxY - minY;
                        
                        rects.erase(rects.begin() + j);
                        changed = true;
                        break; 
                    }
                }
                if (changed) break;
            }
        }

        std::sort(rects.begin(), rects.end(), [](const Rect& a, const Rect& b) {
            return a.x < b.x; 
        });

        for (size_t i = 0; i < rects.size(); ++i) {
            m_newSprites.push_back(std::make_shared<SpriteDefinition>("sprite_" + std::to_string(i + 1), rects[i]));
        }
        m_executed = true;
    }

    m_project->SetSprites(m_newSprites);
}

void MergeSpritesCommand::Undo() {
    if (m_project) {
        m_project->SetSprites(m_oldSprites);
    }
}

}