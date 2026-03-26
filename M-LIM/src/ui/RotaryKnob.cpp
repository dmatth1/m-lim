#include "RotaryKnob.h"
#include "Colours.h"

// 270-degree arc: 7:30 position to 4:30 position (Pro-L 2 style)
static constexpr float kRotaryStartAngle = juce::MathConstants<float>::pi * 1.25f;  // -135° from 12 o'clock
static constexpr float kRotaryEndAngle   = juce::MathConstants<float>::pi * 2.75f;  // +135° from 12 o'clock

RotaryKnob::RotaryKnob()
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (kRotaryStartAngle, kRotaryEndAngle, true);
    slider.onValueChange = [this] { updateCachedValue(); sliderValueChanged(); };
    updateCachedValue();
    addAndMakeVisible (slider);
}

void RotaryKnob::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Reserve space: label row above, value row below
    const float labelH     = 13.0f;
    const float valueH     = 13.0f;
    const float textHeight = labelH + valueH;
    const float knobSize   = juce::jmin (bounds.getWidth(), bounds.getHeight() - textHeight);
    const float knobX      = bounds.getCentreX() - knobSize * 0.5f;
    const float knobY      = bounds.getY() + labelH;

    const float centreX = knobX + knobSize * 0.5f;
    const float centreY = knobY + knobSize * 0.5f;
    const float radius  = knobSize * 0.5f - 4.0f;

    // Knob face — 3D sphere gradient (lighter top-left, darker bottom-right)
    const float faceRadius = radius * 0.80f;
    {
        juce::ColourGradient gradient (
            MLIMColours::knobFaceHighlight,
            centreX - faceRadius * 0.3f, centreY - faceRadius * 0.3f,
            MLIMColours::knobFaceShadow,
            centreX + faceRadius * 0.4f, centreY + faceRadius * 0.4f,
            true);
        g.setGradientFill (gradient);
        g.fillEllipse (centreX - faceRadius, centreY - faceRadius,
                       faceRadius * 2.0f, faceRadius * 2.0f);
    }

    // Track background arc
    {
        juce::Path trackPath;
        trackPath.addArc (centreX - radius, centreY - radius,
                          radius * 2.0f, radius * 2.0f,
                          kRotaryStartAngle, kRotaryEndAngle, true);
        g.setColour (MLIMColours::panelBorder);
        g.strokePath (trackPath, juce::PathStrokeType (2.0f,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Value arc
    {
        const double proportion = (slider.getValue() - slider.getMinimum())
                                  / (slider.getMaximum() - slider.getMinimum());
        const float angle = kRotaryStartAngle
                            + static_cast<float>(proportion)
                              * (kRotaryEndAngle - kRotaryStartAngle);

        if (angle > kRotaryStartAngle)
        {
            juce::Path arcPath;
            arcPath.addArc (centreX - radius, centreY - radius,
                            radius * 2.0f, radius * 2.0f,
                            kRotaryStartAngle, angle, true);
            g.setColour (MLIMColours::knobArc);
            g.strokePath (arcPath, juce::PathStrokeType (2.0f,
                          juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // Pointer tick
    {
        const double proportion = (slider.getValue() - slider.getMinimum())
                                  / (slider.getMaximum() - slider.getMinimum());
        const float angle = kRotaryStartAngle
                            + static_cast<float>(proportion)
                              * (kRotaryEndAngle - kRotaryStartAngle);
        const float pointerLength    = faceRadius * 0.50f;
        const float pointerThickness = 2.5f;

        juce::Path pointer;
        pointer.addRectangle (-pointerThickness * 0.5f, -faceRadius,
                               pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle)
                                    .translated (centreX, centreY));
        g.setColour (MLIMColours::knobPointer);
        g.fillPath (pointer);
    }

    // Label text ABOVE the knob
    g.setColour (MLIMColours::textSecondary);
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall, juce::Font::bold));
    g.drawFittedText (labelText,
                      juce::Rectangle<int> ((int)knobX, (int)bounds.getY(),
                                            (int)knobSize, (int)labelH),
                      juce::Justification::centred, 1);

    // Value row: [min label] [current value] [max label]
    const float valueRowY = (float)(int)(knobY + knobSize + 2);
    const float valueRowH = valueH;
    const float colW = knobSize / 3.0f;

    // Min label (left third)
    g.setColour (MLIMColours::textSecondary.withAlpha (0.65f));
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall - 1.0f));
    juce::String minStr = juce::String (slider.getMinimum(), 0) + " " + suffixText;
    g.drawFittedText (minStr,
                      juce::Rectangle<int> ((int)knobX, (int)valueRowY, (int)colW, (int)valueRowH),
                      juce::Justification::centredLeft, 1);

    // Current value (centre third)
    g.setColour (MLIMColours::textPrimary);
    g.setFont (juce::Font (MLIMColours::kFontSizeMedium));
    g.drawFittedText (cachedValueStr_,
                      juce::Rectangle<int> ((int)(knobX + colW), (int)valueRowY,
                                            (int)colW, (int)valueRowH),
                      juce::Justification::centred, 1);

    // Max label (right third)
    g.setColour (MLIMColours::textSecondary.withAlpha (0.65f));
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall - 1.0f));
    juce::String maxStr = juce::String (slider.getMaximum(), 0) + " " + suffixText;
    g.drawFittedText (maxStr,
                      juce::Rectangle<int> ((int)(knobX + 2.0f * colW), (int)valueRowY,
                                            (int)colW, (int)valueRowH),
                      juce::Justification::centredRight, 1);
}

void RotaryKnob::resized()
{
    const auto bounds = getLocalBounds();
    const float labelH     = 13.0f;
    const float textHeight = labelH + 13.0f;
    const float knobSize   = juce::jmin ((float)bounds.getWidth(),
                                         (float)bounds.getHeight() - textHeight);
    const int knobX        = bounds.getCentreX() - (int)(knobSize * 0.5f);
    slider.setBounds (knobX, bounds.getY() + (int)labelH, (int)knobSize, (int)knobSize);
}

void RotaryKnob::setRange (float min, float max, float step)
{
    slider.setRange (min, max, step);
    repaint();
}

void RotaryKnob::setValue (float val)
{
    slider.setValue (val, juce::dontSendNotification);
    repaint();
}

float RotaryKnob::getValue() const
{
    return (float)slider.getValue();
}

void RotaryKnob::setLabel (const juce::String& label)
{
    labelText = label;
    repaint();
}

void RotaryKnob::setSuffix (const juce::String& suffix)
{
    suffixText = suffix;
    updateCachedValue();
    repaint();
}

void RotaryKnob::sliderValueChanged()
{
    repaint();
    if (onValueChange)
        onValueChange ((float)slider.getValue());
}

void RotaryKnob::updateCachedValue()
{
    cachedValueStr_ = juce::String (slider.getValue(), 1) + " " + suffixText;
}
