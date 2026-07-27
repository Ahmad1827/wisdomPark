#include "Commands/CommandHistory.h"

namespace StudioCore {

void CommandHistory::ExecuteCommand(std::unique_ptr<ICommand> command) {
    command->Execute();
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();
}

void CommandHistory::Undo() {
    if (m_undoStack.empty()) return;
    auto command = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    command->Undo();
    m_redoStack.push_back(std::move(command));
}

void CommandHistory::Redo() {
    if (m_redoStack.empty()) return;
    auto command = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    command->Execute();
    m_undoStack.push_back(std::move(command));
}

void CommandHistory::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

}