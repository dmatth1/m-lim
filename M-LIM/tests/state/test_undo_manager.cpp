#include "catch2/catch_amalgamated.hpp"
#include <juce_audio_processors/juce_audio_processors.h>

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
    juce::UndoManager manager;

    int value = 0;

    manager.beginNewTransaction();
    manager.perform(new SetValueAction(value, 42));

    REQUIRE(value == 42);

    bool result = manager.undo();
    REQUIRE(result == true);
    REQUIRE(value == 0);
}

TEST_CASE("test_redo_restores_change", "[UndoManager]")
{
    juce::UndoManager manager;

    int value = 0;

    manager.beginNewTransaction();
    manager.perform(new SetValueAction(value, 99));

    REQUIRE(value == 99);

    manager.undo();
    REQUIRE(value == 0);

    bool result = manager.redo();
    REQUIRE(result == true);
    REQUIRE(value == 99);
}

TEST_CASE("test_can_undo_redo_flags", "[UndoManager]")
{
    juce::UndoManager manager;

    int value = 0;

    // Initially nothing to undo or redo
    REQUIRE(manager.canUndo() == false);
    REQUIRE(manager.canRedo() == false);

    manager.beginNewTransaction();
    manager.perform(new SetValueAction(value, 7));

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

TEST_CASE("test_multi_step_undo", "[UndoManager]")
{
    juce::UndoManager manager;

    int value = 0;
    const int steps = 5;
    const int vals[steps] = { 10, 20, 30, 40, 50 };

    // Perform 5 separate transactions
    for (int i = 0; i < steps; ++i)
    {
        manager.beginNewTransaction();
        manager.perform(new SetValueAction(value, vals[i]));
        REQUIRE(value == vals[i]);
    }

    // Undo each step and verify the intermediate values
    for (int i = steps - 1; i >= 1; --i)
    {
        manager.undo();
        REQUIRE(value == vals[i - 1]);
    }

    // Final undo returns to initial value
    manager.undo();
    REQUIRE(value == 0);
}

TEST_CASE("test_new_action_clears_redo_stack", "[UndoManager]")
{
    juce::UndoManager manager;

    int value = 0;

    manager.beginNewTransaction();
    manager.perform(new SetValueAction(value, 42));

    manager.undo();
    REQUIRE(manager.canRedo() == true);

    // Performing a new action after undo should clear the redo stack
    manager.beginNewTransaction();
    manager.perform(new SetValueAction(value, 99));

    REQUIRE(manager.canRedo() == false);
    REQUIRE(value == 99);
}

TEST_CASE("test_transaction_grouping", "[UndoManager]")
{
    juce::UndoManager manager;

    int valueA = 0;
    int valueB = 0;

    // Two performs inside one transaction should be a single undo step
    manager.beginNewTransaction();
    manager.perform(new SetValueAction(valueA, 10));
    manager.perform(new SetValueAction(valueB, 20));

    REQUIRE(valueA == 10);
    REQUIRE(valueB == 20);

    // A single undo should revert both
    manager.undo();
    REQUIRE(valueA == 0);
    REQUIRE(valueB == 0);

    // Nothing more to undo
    REQUIRE(manager.canUndo() == false);
}

TEST_CASE("test_many_actions_no_crash", "[UndoManager]")
{
    juce::UndoManager manager;

    int value = 0;
    const int count = 100;

    // Create 100 transactions
    for (int i = 1; i <= count; ++i)
    {
        manager.beginNewTransaction();
        manager.perform(new SetValueAction(value, i));
    }
    REQUIRE(value == count);

    // Undo all of them one by one without crash
    while (manager.canUndo())
        manager.undo();

    REQUIRE(value == 0);
}
