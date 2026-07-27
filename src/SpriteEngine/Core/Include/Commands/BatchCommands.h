#pragma once
#include "Commands/ICommand.h"
#include "DataModels/SpriteDefinition.h"
#include <string>
#include <vector>
#include <memory>

namespace StudioCore {

class Project;

enum class BatchOp {
    FlipHorizontal,
    FlipVertical,
    Rotate90,
    ResetPivot,
    ResetBaseline,
    NormalizeBaseline
};

enum class AlignOp {
    Left,
    Right,
    Top,
    Bottom,
    CenterH,
    CenterV
};

class BatchOperationCommand : public ICommand {
public:
    BatchOperationCommand(std::shared_ptr<Project> project, const std::vector<std::string>& spriteIds, BatchOp op);
    void Execute() override;
    void Undo() override;

private:
    std::shared_ptr<Project> m_project;
    std::vector<std::string> m_spriteIds;
    BatchOp m_op;

    struct Backup { 
        Point pivot; 
        float baseline; 
    };
    std::vector<Backup> m_backups;
};

class AlignSpritesCommand : public ICommand {
public:
    AlignSpritesCommand(std::shared_ptr<Project> project, const std::vector<std::string>& spriteIds, AlignOp op);
    void Execute() override;
    void Undo() override;

private:
    std::shared_ptr<Project> m_project;
    std::vector<std::string> m_spriteIds;
    AlignOp m_op;
    std::vector<Point> m_oldPivots;
};

}