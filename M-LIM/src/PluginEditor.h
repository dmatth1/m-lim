#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class MLIMAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MLIMAudioProcessorEditor (MLIMAudioProcessor& p);
    ~MLIMAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MLIMAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLIMAudioProcessorEditor)
};
