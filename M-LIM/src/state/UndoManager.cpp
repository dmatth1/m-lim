#include "state/UndoManager.h"

void UndoManager::beginNewTransaction()
{
    juceUndoManager.beginNewTransaction();
}

bool UndoManager::undo()
{
    return juceUndoManager.undo();
}

bool UndoManager::redo()
{
    return juceUndoManager.redo();
}

bool UndoManager::canUndo() const
{
    return juceUndoManager.canUndo();
}

bool UndoManager::canRedo() const
{
    return juceUndoManager.canRedo();
}
