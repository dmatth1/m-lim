#include "RotaryKnob.h"
#include "Colours.h"

static constexpr float kRotaryStartAngle = juce::MathConstants<float>::pi * 1.25f;
static constexpr float kRotaryEndAngle   = juce::MathConstants<float>::pi * 2.75f;

RotaryKnob::RotaryKnob()
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (kRotaryStartAngle, kRotaryEndAngle, true);
    slider.onValueChange = [this] { sliderValueChanged(); };
    addAndMakeVisible (slider);
}

void RotaryKnob::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Reserve space at the bottom for label and value text
    const float textHeight = 28.0f;
    const float knobSize   = juce::jmin (bounds.getWidth(), bounds.getHeight() - textHeight);
    const float knobX      = bounds.getCentreX() - knobSize * 0.5f;
    const float knobY      = bounds.getY();

    const float centreX = knobX + knobSize * 0.5f;
    const float centreY = knobY + knobSize * 0.5f;
    const float radius  = knobSize * 0.5f - 4.0f;

    // Knob face
    const float faceRadius = radius * 0.78f;
    g.setColour (MLIMColours::knobFace);
    g.fillEllipse (centreX - faceRadius, centreY - faceRadius,
                   faceRadius * 2.0f, faceRadius * 2.0f);

    // Graduation tick marks (behind arcs, above knob face)
    {
        const int numTicks = 25;  // 25 ticks = 24 intervals, major every 6th
        for (int i = 0; i < numTicks; ++i)
        {
            const float proportion = static_cast<float> (i) / static_cast<float> (numTicks - 1);
            const float angle = kRotaryStartAngle + proportion * (kRotaryEndAngle - kRotaryStartAngle);

            const bool isMajor = (i % 6 == 0);
            const float outerR    = radius * 0.97f;
            const float innerR    = outerR - (isMajor ? 5.0f : 3.0f);
            const float thickness = isMajor ? 1.5f : 1.0f;
            const float alpha     = isMajor ? 0.6f : 0.4f;

            const float sinA = std::sin (angle);
            const float cosA = std::cos (angle);

            g.setColour (juce::Colour (0xff555555).withAlpha (alpha));
            g.drawLine (centreX + sinA * innerR, centreY - cosA * innerR,
                        centreX + sinA * outerR,  centreY - cosA * outerR,
                        thickness);
        }
    }

    // Track background arc
    {
        juce::Path trackPath;
        trackPath.addArc (centreX - radius, centreY - radius,
                          radius * 2.0f, radius * 2.0f,
                          kRotaryStartAngle, kRotaryEndAngle, true);
        g.setColour (MLIMColours::panelBorder);
        g.strokePath (trackPath, juce::PathStrokeType (3.0f,
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
            g.strokePath (arcPath, juce::PathStrokeType (3.0f,
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
        const float pointerLength    = faceRadius * 0.55f;
        const float pointerThickness = 2.5f;

        juce::Path pointer;
        pointer.addRectangle (-pointerThickness * 0.5f, -faceRadius,
                               pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle)
                                    .translated (centreX, centreY));
        g.setColour (MLIMColours::knobPointer);
        g.fillPath (pointer);
    }

    // Label text (below knob)
    const float textY = knobY + knobSize + 2.0f;
    g.setColour (MLIMColours::textSecondary);
    g.setFont (juce::Font (11.0f));
    g.drawFittedText (labelText,
                      juce::Rectangle<int> ((int)knobX, (int)textY,
                                            (int)knobSize, 13),
                      juce::Justification::centred, 1);

    // Value + suffix text
    juce::String valueStr = juce::String (slider.getValue(), 1) + " " + suffixText;
    g.setColour (MLIMColours::textPrimary);
    g.setFont (juce::Font (11.0f));
    g.drawFittedText (valueStr,
                      juce::Rectangle<int> ((int)knobX, (int)textY + 13,
                                            (int)knobSize, 13),
                      juce::Justification::centred, 1);
}

void RotaryKnob::resized()
{
    const auto bounds = getLocalBounds();
    const float textHeight = 28.0f;
    const float knobSize   = juce::jmin ((float)bounds.getWidth(),
                                         (float)bounds.getHeight() - textHeight);
    const int knobX        = bounds.getCentreX() - (int)(knobSize * 0.5f);
    slider.setBounds (knobX, bounds.getY(), (int)knobSize, (int)knobSize);
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
    repaint();
}

void RotaryKnob::sliderValueChanged()
{
    repaint();
    if (onValueChange)
        onValueChange ((float)slider.getValue());
}
