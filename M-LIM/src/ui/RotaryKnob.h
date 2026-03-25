#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class RotaryKnob : public juce::Component
{
public:
    RotaryKnob();
    ~RotaryKnob() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setRange (float min, float max, float step);
    void setValue (float val);
    float getValue() const;
    void setLabel (const juce::String& label);
    void setSuffix (const juce::String& suffix);

    juce::Slider& getSlider() { return slider; }

    std::function<void(float)> onValueChange;

private:
    juce::Slider slider;
    juce::String labelText;
    juce::String suffixText;
    juce::String cachedValueStr_;

    void sliderValueChanged();
    void updateCachedValue();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
};
