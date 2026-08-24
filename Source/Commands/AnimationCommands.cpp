#include "Commands/AnimationCommands.h"
#include "DataModels/Project.h"
#include "DataModels/AnimationGroup.h"

namespace StudioCore {

CreateAnimationCommand::CreateAnimationCommand(std::shared_ptr<Project> project, std::string id, std::string name)
    : m_project(std::move(project)), m_id(std::move(id)), m_name(std::move(name)) {}

void CreateAnimationCommand::Execute() {
    auto anim = std::make_shared<AnimationGroup>(m_id, m_name);
    m_project->AddAnimationGroup(std::move(anim));
}

void CreateAnimationCommand::Undo() {
    m_project->RemoveAnimationGroup(m_id);
}

DeleteAnimationCommand::DeleteAnimationCommand(std::shared_ptr<Project> project, std::string id)
    : m_project(std::move(project)), m_id(std::move(id)) {}

void DeleteAnimationCommand::Execute() {
    m_savedAnim = m_project->GetAnimationById(m_id);
    m_project->RemoveAnimationGroup(m_id);
}

void DeleteAnimationCommand::Undo() {
    if (m_savedAnim) {
        m_project->AddAnimationGroup(m_savedAnim);
    }
}

ModifyFramesCommand::ModifyFramesCommand(std::shared_ptr<Project> project, std::string animId, std::vector<std::string> newFrames)
    : m_project(std::move(project)), m_animId(std::move(animId)), m_newFrames(std::move(newFrames)) {}

void ModifyFramesCommand::Execute() {
    auto anim = m_project->GetAnimationById(m_animId);
    if (anim) {
        m_oldFrames = anim->GetFrames();
        anim->SetFrames(m_newFrames);
    }
}

void ModifyFramesCommand::Undo() {
    auto anim = m_project->GetAnimationById(m_animId);
    if (anim) {
        anim->SetFrames(m_oldFrames);
    }
}

EditAnimationSettingsCommand::EditAnimationSettingsCommand(std::shared_ptr<Project> project, std::string animId, std::string newName, float newFps, bool newLoop)
    : m_project(std::move(project)), m_animId(std::move(animId)), m_newName(std::move(newName)), m_newFps(newFps), m_newLoop(newLoop) {}

void EditAnimationSettingsCommand::Execute() {
    auto anim = m_project->GetAnimationById(m_animId);
    if (anim) {
        m_oldName = anim->GetName();
        m_oldFps = anim->GetFPS();
        m_oldLoop = anim->IsLooping();
        
        anim->SetName(m_newName);
        anim->SetFPS(m_newFps);
        anim->SetLooping(m_newLoop);
    }
}

void EditAnimationSettingsCommand::Undo() {
    auto anim = m_project->GetAnimationById(m_animId);
    if (anim) {
        anim->SetName(m_oldName);
        anim->SetFPS(m_oldFps);
        anim->SetLooping(m_oldLoop);
    }
}

}