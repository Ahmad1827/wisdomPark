#pragma once
#include "Commands/ICommand.h"
#include "DataModels/Project.h"
#include "DataModels/SpriteDefinition.h"
#include <memory>
#include <string>

namespace StudioCore {

class MoveSpriteCommand : public ICommand {
public:
    MoveSpriteCommand(std::shared_ptr<Project> project,
                      const std::string& spriteId,
                      const Rect& oldRect,
                      const Rect& newRect)
        : m_project(project), m_spriteId(spriteId), m_oldRect(oldRect), m_newRect(newRect) {}

    void Execute() override {
        ApplyRect(m_newRect);
    }

    void Undo() override {
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
    std::string m_spriteId;
    Rect m_oldRect;
    Rect m_newRect;
};

}