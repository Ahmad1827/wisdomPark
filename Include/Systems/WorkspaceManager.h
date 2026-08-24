#pragma once
#include <memory>

namespace StudioCore {

class Project;

class WorkspaceManager {
public:
    WorkspaceManager() = default;

    void CreateNewProject();
    bool HasActiveProject() const;
    std::shared_ptr<Project> GetActiveProject() const;

private:
    std::shared_ptr<Project> m_activeProject;
};

}