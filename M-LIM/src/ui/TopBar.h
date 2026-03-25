#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * TopBar — horizontal toolbar at the top of the plugin window.
 *
 * Layout:
 *   [M-LIM logo]  |  [◀] [preset name] [▶]  |  [A/B] [A→B] [Undo] [Redo]
 *
 * Height: ~30 px. Callbacks fire on the message thread.
 */
class TopBar : public juce::Component
{
public:
    TopBar();
    ~TopBar() override = default;

    /** Update the displayed preset name. */
    void setPresetName (const juce::String& name);

    /** Returns the currently displayed preset name. */
    juce::String getPresetName() const;

    // ── Callbacks ─────────────────────────────────────────────────────────
    std::function<void()> onUndo;
    std::function<void()> onRedo;
    std::function<void()> onABToggle;
    std::function<void()> onABCopy;
    std::function<void()> onPresetPrev;
    std::function<void()> onPresetNext;

    // Component overrides
    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    juce::Label      logoLabel_;
    juce::TextButton prevButton_    { "<" };
    juce::Label      presetLabel_;
    juce::TextButton nextButton_    { ">" };
    juce::TextButton abToggleButton_{ "A/B" };
    juce::TextButton abCopyButton_  { "Copy" };
    juce::TextButton undoButton_    { juce::CharPointer_UTF8 ("\xe2\x86\xa9") };  // ↩
    juce::TextButton redoButton_    { juce::CharPointer_UTF8 ("\xe2\x86\xaa") };  // ↪

    static void styleBarButton (juce::TextButton& btn);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBar)
};
