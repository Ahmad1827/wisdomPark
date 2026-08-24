#pragma once
#include "Commands/ICommand.h"
#include <memory>
#include <vector>

namespace StudioCore {

class Project;
class SourceTexture;
class SpriteDefinition;

class FlipHorizontalCommand : public ICommand {
public:
    FlipHorizontalCommand(std::shared_ptr<Project> project);
    void Execute() override;
    void Undo() override;

private:
    std::shared_ptr<Project> m_project;
    std::shared_ptr<const SourceTexture> m_oldTexture;
    std::shared_ptr<const SourceTexture> m_newTexture;
};

}