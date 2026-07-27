#pragma once
#include "ICommand.h"
#include <vector>
#include <memory>

namespace StudioCore {

class CommandHistory {
public:
    CommandHistory() = default;

    void ExecuteCommand(std::unique_ptr<ICommand> command);
    void Undo();
    void Redo();
    void Clear();

private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;
};

}