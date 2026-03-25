#include "catch2/catch_amalgamated.hpp"
#include "state/UndoManager.h"

// Helper: a simple undoable action that tracks an integer value
class SetValueAction : public juce::UndoableAction
{
public:
    SetValueAction(int& target, int newValue)
        : target(target), newValue(newValue), oldValue(target) {}

    bool perform() override
    {
        target = newValue;
        return true;
    }

    bool undo() override
    {
        target = oldValue;
        return true;
    }

private:
    int& target;
    int newValue;
    int oldValue;
};

TEST_CASE("test_undo_reverts_change", "[UndoManager]")
{
    UndoManager manager;
    auto& juce = manager.getJuceUndoManager();

    int value = 0;

    manager.beginNewTransaction();
    juce.perform(new SetValueAction(value, 42));

    REQUIRE(value == 42);

    bool result = manager.undo();
    REQUIRE(result == true);
    REQUIRE(value == 0);
}

TEST_CASE("test_redo_restores_change", "[UndoManager]")
{
    UndoManager manager;
    auto& juce = manager.getJuceUndoManager();

    int value = 0;

    manager.beginNewTransaction();
    juce.perform(new SetValueAction(value, 99));

    REQUIRE(value == 99);

    manager.undo();
    REQUIRE(value == 0);

    bool result = manager.redo();
    REQUIRE(result == true);
    REQUIRE(value == 99);
}

TEST_CASE("test_can_undo_redo_flags", "[UndoManager]")
{
    UndoManager manager;
    auto& juce = manager.getJuceUndoManager();

    int value = 0;

    // Initially nothing to undo or redo
    REQUIRE(manager.canUndo() == false);
    REQUIRE(manager.canRedo() == false);

    manager.beginNewTransaction();
    juce.perform(new SetValueAction(value, 7));

    // After a change: can undo, cannot redo
    REQUIRE(manager.canUndo() == true);
    REQUIRE(manager.canRedo() == false);

    manager.undo();

    // After undo: cannot undo again (only one transaction), can redo
    REQUIRE(manager.canUndo() == false);
    REQUIRE(manager.canRedo() == true);

    manager.redo();

    // After redo: can undo again, cannot redo
    REQUIRE(manager.canUndo() == true);
    REQUIRE(manager.canRedo() == false);
}
