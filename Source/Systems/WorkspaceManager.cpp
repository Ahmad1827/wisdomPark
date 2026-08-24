#include "Systems/WorkspaceManager.h"
#include "DataModels/Project.h"

namespace StudioCore {

void WorkspaceManager::CreateNewProject() {
    m_activeProject = std::make_shared<Project>();
}

bool WorkspaceManager::HasActiveProject() const {
    return m_activeProject != nullptr;
}

std::shared_ptr<Project> WorkspaceManager::GetActiveProject() const {
    return m_activeProject;
}

}