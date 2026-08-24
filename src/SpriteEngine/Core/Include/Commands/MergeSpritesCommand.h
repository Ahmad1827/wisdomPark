#pragma once
#include "Commands/ICommand.h"
#include <memory>
#include <vector>

namespace StudioCore {

class Project;
class SpriteDefinition;

class MergeSpritesCommand : public ICommand {
public:
    MergeSpritesCommand(std::shared_ptr<Project> project);
    void Execute() override;
    void Undo() override;

private:
    std::shared_ptr<Project> m_project;
    std::vector<std::shared_ptr<SpriteDefinition>> m_oldSprites;
    std::vector<std::shared_ptr<SpriteDefinition>> m_newSprites;
    bool m_executed = false;
};

}