#include "Commands/BatchCommands.h"
#include "DataModels/Project.h"
#include "DataModels/SpriteDefinition.h"

namespace StudioCore {

BatchOperationCommand::BatchOperationCommand(std::shared_ptr<Project> project, const std::vector<std::string>& spriteIds, BatchOp op)
    : m_project(project), m_spriteIds(spriteIds), m_op(op) {}

void BatchOperationCommand::Execute() {
    if (!m_project) return;
    m_backups.clear();

    float avgBaseline = 0.0f;
    if (m_op == BatchOp::NormalizeBaseline && !m_spriteIds.empty()) {
        size_t count = 0;
        for (const auto& id : m_spriteIds) {
            auto s = m_project->GetSpriteById(id);
            if (s) {
                avgBaseline += s->GetBaseline();
                count++;
            }
        }
        if (count > 0) avgBaseline /= static_cast<float>(count);
    }

    for (const auto& id : m_spriteIds) {
        auto s = m_project->GetSpriteById(id);
        if (!s) continue;

        Point curPivot = s->GetPivot();
        m_backups.push_back({curPivot, s->GetBaseline()});

        if (m_op == BatchOp::FlipHorizontal) {
            s->SetPivot({1.0f - curPivot.x, curPivot.y});
        } else if (m_op == BatchOp::FlipVertical) {
            s->SetPivot({curPivot.x, 1.0f - curPivot.y});
        } else if (m_op == BatchOp::ResetPivot) {
            s->SetPivot({0.5f, 0.5f});
        } else if (m_op == BatchOp::ResetBaseline) {
            s->SetBaseline(static_cast<float>(s->GetSourceRect().height));
        } else if (m_op == BatchOp::NormalizeBaseline) {
            s->SetBaseline(avgBaseline);
        }
    }
}

void BatchOperationCommand::Undo() {
    if (!m_project) return;
    for (size_t i = 0; i < m_spriteIds.size() && i < m_backups.size(); ++i) {
        auto s = m_project->GetSpriteById(m_spriteIds[i]);
        if (s) {
            s->SetPivot(m_backups[i].pivot);
            s->SetBaseline(m_backups[i].baseline);
        }
    }
}

AlignSpritesCommand::AlignSpritesCommand(std::shared_ptr<Project> project, const std::vector<std::string>& spriteIds, AlignOp op)
    : m_project(project), m_spriteIds(spriteIds), m_op(op) {}

void AlignSpritesCommand::Execute() {
    if (!m_project || m_spriteIds.empty()) return;
    m_oldPivots.clear();

    for (const auto& id : m_spriteIds) {
        auto s = m_project->GetSpriteById(id);
        if (!s) continue;

        Point curPivot = s->GetPivot();
        m_oldPivots.push_back(curPivot);

        Point p = curPivot;
        if (m_op == AlignOp::Left) p.x = 0.0f;
        else if (m_op == AlignOp::Right) p.x = 1.0f;
        else if (m_op == AlignOp::Top) p.y = 0.0f;
        else if (m_op == AlignOp::Bottom) p.y = 1.0f;
        else if (m_op == AlignOp::CenterH) p.x = 0.5f;
        else if (m_op == AlignOp::CenterV) p.y = 0.5f;

        s->SetPivot(p);
    }
}

void AlignSpritesCommand::Undo() {
    if (!m_project) return;
    for (size_t i = 0; i < m_spriteIds.size() && i < m_oldPivots.size(); ++i) {
        auto s = m_project->GetSpriteById(m_spriteIds[i]);
        if (s) {
            s->SetPivot(m_oldPivots[i]);
        }
    }
}

}