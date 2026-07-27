#pragma once
#include "Commands/ICommand.h"
#include "DataModels/SpriteDefinition.h"
#include <vector>
#include <memory>
#include <string>

namespace StudioCore {

class Project;

enum class EditType {
    Pivot,
    Baseline
};

class EditMetadataCommand : public ICommand {
public:
    EditMetadataCommand(std::shared_ptr<Project> project, const std::vector<std::string>& spriteIds, EditType type, Point newPivot, float newBaseline);

    void Execute() override;
    void Undo() override;

private:
    std::shared_ptr<Project> m_project;
    std::vector<std::string> m_spriteIds;
    EditType m_type;
    Point m_newPivot;
    float m_newBaseline;
    std::vector<Point> m_oldPivots;
    std::vector<float> m_oldBaselines;
};

}