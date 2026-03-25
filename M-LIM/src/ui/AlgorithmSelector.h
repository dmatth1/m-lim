#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

/**
 * Algorithm selector UI component — a styled dropdown button that shows the
 * current limiter algorithm name and opens a popup menu on click.
 *
 * Supports APVTS ComboBoxAttachment for the "algorithm" parameter.
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

    static constexpr int NUM_ALGORITHMS = 8;
    static const char* const ALGORITHM_NAMES[NUM_ALGORITHMS];

private:
    juce::ComboBox comboBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmSelector)
};
