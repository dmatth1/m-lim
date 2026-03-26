#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

/**
 * Algorithm selector UI component — compact single-row navigation widget:
 * [ ‹ ] [ ALGORITHM NAME ] [ › ]
 *
 * The underlying juce::ComboBox is kept hidden and acts as the APVTS
 * attachment point (via getComboBox()). Prev/next buttons cycle through
 * algorithms 0–7 with wrap-around.
 */
class AlgorithmSelector : public juce::Component
{
public:
    AlgorithmSelector();
    ~AlgorithmSelector() override = default;

    /** Set the current algorithm by zero-based index (0-7). */
    void setAlgorithm(int index);

    /** Returns the current zero-based algorithm index. */
    int getAlgorithm() const;

    /** Called whenever the user selects a new algorithm. */
    std::function<void(int)> onAlgorithmChanged;

    /** Returns the underlying ComboBox for APVTS attachment. */
    juce::ComboBox& getComboBox() { return comboBox; }

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    /** Hidden ComboBox — APVTS attachment point only. */
    juce::ComboBox comboBox;

    /** Navigation buttons. */
    juce::TextButton prevButton_;
    juce::TextButton nextButton_;

    /** Displays the current algorithm name. */
    juce::Label nameLabel_;

    /** Refresh label text to reflect current comboBox selection. */
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmSelector)
};
