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

TEST_CASE("test_history_limit_drops_oldest_no_crash", "[UndoManager]")
{
    // UndoableAction::getSizeInUnits() returns 10 by default.
    // To keep exactly `limit` transactions, set maxUnitsToKeep = limit * 10.
    const int limit = 5;
    juce::UndoManager manager(limit * 10, 0);

    int value = 0;
    const int totalActions = 20;

    // Perform 20 transactions — only the last `limit` should be retained
    for (int i = 1; i <= totalActions; ++i)
    {
        manager.beginNewTransaction();
        manager.perform(new SetValueAction(value, i));
    }
    REQUIRE(value == totalActions);

    // There should be something to undo (oldest entries dropped, not all)
    REQUIRE(manager.canUndo() == true);

    // Count how many undos are available — must not exceed the limit
    int undoCount = 0;
    while (manager.canUndo())
    {
        manager.undo();
        ++undoCount;
    }

    // At most `limit` undos should have been available
    REQUIRE(undoCount <= limit);
    // And at least one undo succeeded (history wasn't entirely lost)
    REQUIRE(undoCount >= 1);
    // No crash — reaching here means we're good
}

TEST_CASE("test_undo_returns_false_when_exhausted", "[UndoManager]")
{
    // Use a large enough limit so all 3 transactions are retained.
    // Default getSizeInUnits() = 10, so 3 transactions cost 30 units.
    juce::UndoManager manager(300, 30);

    int value = 0;

    // Perform a few transactions
    for (int i = 1; i <= 3; ++i)
    {
        manager.beginNewTransaction();
        manager.perform(new SetValueAction(value, i));
    }

    // Undo all available transactions
    while (manager.canUndo())
        manager.undo();

    REQUIRE(manager.canUndo() == false);

    // Calling undo() when history is exhausted must return false, not crash
    bool result = manager.undo();
    REQUIRE(result == false);

    // Value should remain at 0 (fully undone)
    REQUIRE(value == 0);
}

TEST_CASE("test_rapid_undo_redo_cycles_stable", "[UndoManager]")
{
    // 10 transactions × 10 units each = 100 units; use 1000 to keep all of them.
    juce::UndoManager manager(1000, 30);

    int value = 0;
    const int actionCount = 10;

    // Perform 10 transactions
    for (int i = 1; i <= actionCount; ++i)
    {
        manager.beginNewTransaction();
        manager.perform(new SetValueAction(value, i));
    }
    REQUIRE(value == actionCount);

    // Perform 50 undo-all / redo-all cycles
    for (int cycle = 0; cycle < 50; ++cycle)
    {
        // Undo all
        while (manager.canUndo())
            manager.undo();

        REQUIRE(manager.canUndo() == false);

        // Redo all
        while (manager.canRedo())
            manager.redo();

        REQUIRE(manager.canRedo() == false);
    }

    // After all cycles, value should be fully redone
    REQUIRE(value == actionCount);
    // No more redos available
    REQUIRE(manager.canRedo() == false);
}

// An action whose perform() always returns false
class FailingAction : public juce::UndoableAction
{
public:
    FailingAction(int& target, int newValue)
        : target(target), newValue(newValue), oldValue(target) {}

    bool perform() override { return false; }
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

TEST_CASE("test_failed_perform_not_added_to_stack", "[UndoManager]")
{
    juce::UndoManager manager;

    int value = 0;

    manager.beginNewTransaction();
    bool result = manager.perform(new FailingAction(value, 42));

    // perform() should return false
    REQUIRE(result == false);
    // Value should be unchanged
    REQUIRE(value == 0);
    // Failed action must not be on the undo stack
    REQUIRE(manager.canUndo() == false);
}

// An action with a known description
class DescribedAction : public juce::UndoableAction
{
public:
    DescribedAction(int& tgt, int nv, const juce::String& desc)
        : target(tgt), newValue(nv), oldValue(tgt), description(desc) {}

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

    int getSizeInUnits() override { return 10; }

private:
    int& target;
    int newValue;
    int oldValue;
    juce::String description;
};

TEST_CASE("test_undo_description_nonempty", "[UndoManager]")
{
    juce::UndoManager manager;

    int value = 0;

    manager.beginNewTransaction("Set to 42");
    manager.perform(new DescribedAction(value, 42, "Set to 42"));

    // JUCE uses the transaction name as the undo description
    juce::String undoDesc = manager.getUndoDescription();
    REQUIRE(undoDesc.isNotEmpty());
    REQUIRE(undoDesc == "Set to 42");
}

TEST_CASE("test_canundo_canredo_alternation", "[UndoManager]")
{
    juce::UndoManager manager(10000, 100);

    int value = 0;
    const int count = 5;

    // Perform 5 actions
    for (int i = 1; i <= count; ++i)
    {
        manager.beginNewTransaction();
        manager.perform(new SetValueAction(value, i * 10));
    }
    REQUIRE(value == 50);
    REQUIRE(manager.canUndo() == true);
    REQUIRE(manager.canRedo() == false);

    // Undo all, checking flags at each step
    for (int i = count; i >= 1; --i)
    {
        REQUIRE(manager.canUndo() == true);
        manager.undo();

        if (i > 1)
        {
            REQUIRE(manager.canUndo() == true);
        }
        else
        {
            REQUIRE(manager.canUndo() == false);
        }
        REQUIRE(manager.canRedo() == true);
    }

    REQUIRE(value == 0);
    REQUIRE(manager.canUndo() == false);
    REQUIRE(manager.canRedo() == true);

    // Redo all, checking flags at each step
    for (int i = 1; i <= count; ++i)
    {
        REQUIRE(manager.canRedo() == true);
        manager.redo();

        if (i < count)
        {
            REQUIRE(manager.canRedo() == true);
        }
        else
        {
            REQUIRE(manager.canRedo() == false);
        }
        REQUIRE(manager.canUndo() == true);
    }

    REQUIRE(value == 50);
    REQUIRE(manager.canUndo() == true);
    REQUIRE(manager.canRedo() == false);
}

TEST_CASE("test_history_limit_oldest_dropped", "[UndoManager]")
{
    // Limit to 3 transactions (3 * 10 units = 30)
    const int limit = 3;
    juce::UndoManager manager(limit * 10, 0);

    int value = 0;
    const int totalActions = 10;

    for (int i = 1; i <= totalActions; ++i)
    {
        manager.beginNewTransaction();
        manager.perform(new SetValueAction(value, i));
    }
    REQUIRE(value == totalActions);

    // Count available undos
    int undoCount = 0;
    while (manager.canUndo())
    {
        manager.undo();
        ++undoCount;
    }

    // Should have at most `limit` undos
    REQUIRE(undoCount <= limit);
    REQUIRE(undoCount >= 1);

    // After undoing everything, canRedo should be true but canUndo false
    REQUIRE(manager.canUndo() == false);
    // Oldest entries were dropped, so we can't undo further
    // canRedo should still be false after the limit test since we just undid
    REQUIRE(manager.canRedo() == true);
}
