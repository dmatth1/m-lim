#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * UndoManager — thin wrapper around juce::UndoManager.
 *
 * Owned by PluginProcessor and passed to APVTS so that parameter changes are
 * automatically recorded as undoable actions.
 */
class UndoManager
{
public:
    UndoManager() = default;

    /** Returns the underlying juce::UndoManager for passing to APVTS. */
    juce::UndoManager& getJuceUndoManager() { return juceUndoManager; }

    /** Starts a new undo transaction (creates an undo point). */
    void beginNewTransaction();

    /** Undoes the last transaction. Returns true if successful. */
    bool undo();

    /** Redoes the last undone transaction. Returns true if successful. */
    bool redo();

    /** Returns true if there is at least one transaction to undo. */
    bool canUndo() const;

    /** Returns true if there is at least one transaction to redo. */
    bool canRedo() const;

private:
    juce::UndoManager juceUndoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoManager)
};
