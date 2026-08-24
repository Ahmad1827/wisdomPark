#pragma once
#include "Commands/ICommand.h"
#include <memory>
#include <vector>

namespace StudioCore {

class Project;
class SourceTexture;

class RemoveArtifactsCommand : public ICommand {
public:
    RemoveArtifactsCommand(std::shared_ptr<Project> project, int startX, int startY);
    void Execute() override;
    void Undo() override;

private:
    std::shared_ptr<Project> m_project;
    std::shared_ptr<SourceTexture> m_oldTexture;
    std::shared_ptr<SourceTexture> m_newTexture;
    int m_startX;
    int m_startY;
    bool m_executed = false;
};

}