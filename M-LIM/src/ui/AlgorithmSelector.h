#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/LimiterAlgorithm.h"
#include <functional>

/**
 * Algorithm selector UI component — 2×4 grid of algorithm buttons.
 *
 * The underlying juce::ComboBox is kept hidden and acts as the APVTS
 * attachment point (via getComboBox()). Eight TextButtons arranged in
 * 2 rows of 4 allow direct selection of any algorithm.
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

    /** 2×4 grid of algorithm selection buttons. */
    juce::TextButton algoButtons_[kNumAlgorithms];

    /** Legacy nav widgets — kept for ABI compatibility but hidden. */
    juce::TextButton prevButton_;
    juce::TextButton nextButton_;
    juce::Label nameLabel_;

    /** Refresh button toggle states to reflect current comboBox selection. */
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmSelector)
};
