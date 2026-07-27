#pragma once
#include "Commands/ICommand.h"
#include <string>
#include <vector>
#include <memory>

namespace StudioCore {

class Project;
class AnimationGroup;

class CreateAnimationCommand : public ICommand {
public:
    CreateAnimationCommand(std::shared_ptr<Project> project, std::string id, std::string name);
    void Execute() override;
    void Undo() override;
private:
    std::shared_ptr<Project> m_project;
    std::string m_id;
    std::string m_name;
};

class DeleteAnimationCommand : public ICommand {
public:
    DeleteAnimationCommand(std::shared_ptr<Project> project, std::string id);
    void Execute() override;
    void Undo() override;
private:
    std::shared_ptr<Project> m_project;
    std::string m_id;
    std::shared_ptr<AnimationGroup> m_savedAnim;
};

class ModifyFramesCommand : public ICommand {
public:
    ModifyFramesCommand(std::shared_ptr<Project> project, std::string animId, std::vector<std::string> newFrames);
    void Execute() override;
    void Undo() override;
private:
    std::shared_ptr<Project> m_project;
    std::string m_animId;
    std::vector<std::string> m_newFrames;
    std::vector<std::string> m_oldFrames;
};

class EditAnimationSettingsCommand : public ICommand {
public:
    EditAnimationSettingsCommand(std::shared_ptr<Project> project, std::string animId, std::string newName, float newFps, bool newLoop);
    void Execute() override;
    void Undo() override;
private:
    std::shared_ptr<Project> m_project;
    std::string m_animId;
    std::string m_newName;
    float m_newFps;
    bool m_newLoop;
    std::string m_oldName;
    float m_oldFps;
    bool m_oldLoop;
};

}