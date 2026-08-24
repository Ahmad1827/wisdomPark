#include "Commands/EditMetadataCommand.h"
#include "DataModels/Project.h"

namespace StudioCore {

EditMetadataCommand::EditMetadataCommand(std::shared_ptr<Project> project, const std::vector<std::string>& spriteIds, EditType type, Point newPivot, float newBaseline)
    : m_project(std::move(project)), m_spriteIds(spriteIds), m_type(type), m_newPivot(newPivot), m_newBaseline(newBaseline) {}

void EditMetadataCommand::Execute() {
    m_oldPivots.clear();
    m_oldBaselines.clear();

    for (const auto& id : m_spriteIds) {
        auto sprite = m_project->GetSpriteById(id);
        if (sprite) {
            m_oldPivots.push_back(sprite->GetPivot());
            m_oldBaselines.push_back(sprite->GetBaseline());

            if (m_type == EditType::Pivot) {
                sprite->SetPivot(m_newPivot);
            } else if (m_type == EditType::Baseline) {
                sprite->SetBaseline(m_newBaseline);
            }
        }
    }
}

void EditMetadataCommand::Undo() {
    size_t index = 0;
    for (const auto& id : m_spriteIds) {
        auto sprite = m_project->GetSpriteById(id);
        if (sprite && index < m_oldPivots.size()) {
            if (m_type == EditType::Pivot) {
                sprite->SetPivot(m_oldPivots[index]);
            } else if (m_type == EditType::Baseline) {
                sprite->SetBaseline(m_oldBaselines[index]);
            }
            index++;
        }
    }
}

}