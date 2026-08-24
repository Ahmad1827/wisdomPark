#pragma once
#include "Commands/ICommand.h"
#include "DataModels/Project.h"
#include "DataModels/SourceTexture.h"
#include "DataModels/SpriteDefinition.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace StudioCore {

class MoveSpriteWithPixelsCommand : public ICommand {
public:
    MoveSpriteWithPixelsCommand(std::shared_ptr<Project> project,
                                std::shared_ptr<SourceTexture> texture,
                                const std::string& spriteId,
                                const Rect& oldRect,
                                const Rect& newRect,
                                const std::vector<uint8_t>& oldPixels,
                                const std::vector<uint8_t>& newPixels)
        : m_project(project),
          m_texture(texture),
          m_spriteId(spriteId),
          m_oldRect(oldRect),
          m_newRect(newRect),
          m_oldPixels(oldPixels),
          m_newPixels(newPixels) {}

    void Execute() override {
        if (m_texture) {
            m_texture->SetPixels(m_newPixels);
        }
        ApplyRect(m_newRect);
    }

    void Undo() override {
        if (m_texture) {
            m_texture->SetPixels(m_oldPixels);
        }
        ApplyRect(m_oldRect);
    }

private:
    void ApplyRect(const Rect& rect) {
        if (!m_project) return;
        auto sprite = m_project->GetSpriteById(m_spriteId);
        if (!sprite) return;

        SpriteDefinition updatedSprite(m_spriteId, rect);
        updatedSprite.SetPivot(sprite->GetPivot());
        updatedSprite.SetBaseline(sprite->GetBaseline());

        m_project->RemoveSprite(m_spriteId);
        m_project->AddSprite(updatedSprite);
    }

    std::shared_ptr<Project> m_project;
    std::shared_ptr<SourceTexture> m_texture;
    std::string m_spriteId;
    Rect m_oldRect;
    Rect m_newRect;
    std::vector<uint8_t> m_oldPixels;
    std::vector<uint8_t> m_newPixels;
};

}