#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

/**
 * Algorithm selector UI component — displays 8 segmented TextButtons (2 rows
 * of 4) representing the 8 limiter algorithms. The selected button is
 * highlighted in accentBlue; all others use the dark buttonBackground colour.
 *
 * The underlying juce::ComboBox is kept hidden and acts as the APVTS
 * attachment point (via getComboBox()). All selection logic routes through it
 * so parameter state stays consistent.
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

    /** One TextButton per algorithm (8 total, laid out as 2 rows of 4). */
    juce::TextButton transparentButton_;
    juce::TextButton punchyButton_;
    juce::TextButton dynamicButton_;
    juce::TextButton aggressiveButton_;
    juce::TextButton allroundButton_;
    juce::TextButton busButton_;
    juce::TextButton safeButton_;
    juce::TextButton modernButton_;

    /** Convenience array pointing to the 8 buttons above (populated in ctor). */
    juce::TextButton* algoButtons_[8];

    /** Refresh all button colours to reflect current comboBox selection. */
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmSelector)
};
