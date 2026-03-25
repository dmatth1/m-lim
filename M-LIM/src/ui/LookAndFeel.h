#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Colours.h"

class MLIMLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MLIMLookAndFeel();
    ~MLIMLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           juce::Slider::SliderStyle style,
                           juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g,
                       int width, int height,
                       bool isButtonDown,
                       int buttonX, int buttonY,
                       int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawButtonBackground (juce::Graphics& g,
                                juce::Button& button,
                                const juce::Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted,
                                bool shouldDrawButtonAsDown) override;

    void drawTooltip (juce::Graphics& g,
                      const juce::String& text,
                      int width, int height) override;
};
