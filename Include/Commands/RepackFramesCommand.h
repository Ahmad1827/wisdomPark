#pragma once
#include "Commands/ICommand.h"
#include <memory>
#include <vector>

namespace StudioCore {

class Project;
class SourceTexture;
class SpriteDefinition;

class RepackFramesCommand : public ICommand {
public:
    RepackFramesCommand(std::shared_ptr<Project> project);
    void Execute() override;
    void Undo() override;

private:
    std::shared_ptr<Project> m_project;
    std::shared_ptr<const SourceTexture> m_oldTexture; // Added const
    std::shared_ptr<const SourceTexture> m_newTexture; // Added const
    std::vector<std::shared_ptr<SpriteDefinition>> m_oldSprites;
    std::vector<std::shared_ptr<SpriteDefinition>> m_newSprites;
};

}