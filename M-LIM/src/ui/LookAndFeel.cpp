#include "LookAndFeel.h"

MLIMLookAndFeel::MLIMLookAndFeel()
{
    // Window / background colours
    setColour (juce::ResizableWindow::backgroundColourId, MLIMColours::background);
    setColour (juce::DocumentWindow::backgroundColourId,  MLIMColours::background);

    // Text colours
    setColour (juce::Label::textColourId,           MLIMColours::textPrimary);
    setColour (juce::Label::backgroundColourId,     juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, MLIMColours::background);
    setColour (juce::TextEditor::textColourId,       MLIMColours::textPrimary);

    // Slider colours
    setColour (juce::Slider::thumbColourId,             MLIMColours::knobPointer);
    setColour (juce::Slider::trackColourId,             MLIMColours::sliderFill);
    setColour (juce::Slider::backgroundColourId,        MLIMColours::knobFace);
    setColour (juce::Slider::textBoxTextColourId,       MLIMColours::textPrimary);
    setColour (juce::Slider::textBoxBackgroundColourId, MLIMColours::displayBackground);
    setColour (juce::Slider::textBoxOutlineColourId,    MLIMColours::panelBorder);

    // ComboBox colours
    setColour (juce::ComboBox::backgroundColourId,   MLIMColours::displayBackground);
    setColour (juce::ComboBox::textColourId,         MLIMColours::textPrimary);
    setColour (juce::ComboBox::outlineColourId,      MLIMColours::panelBorder);
    setColour (juce::ComboBox::arrowColourId,        MLIMColours::textSecondary);
    setColour (juce::ComboBox::buttonColourId,       MLIMColours::displayBackground);
    setColour (juce::PopupMenu::backgroundColourId,  MLIMColours::background);
    setColour (juce::PopupMenu::textColourId,        MLIMColours::textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, MLIMColours::accentBlue);
    setColour (juce::PopupMenu::highlightedTextColourId,       MLIMColours::textPrimary);

    // Button colours
    setColour (juce::TextButton::buttonColourId,       MLIMColours::displayBackground);
    setColour (juce::TextButton::buttonOnColourId,     MLIMColours::accentBlue);
    setColour (juce::TextButton::textColourOffId,      MLIMColours::textPrimary);
    setColour (juce::TextButton::textColourOnId,       MLIMColours::textPrimary);

    // ToggleButton
    setColour (juce::ToggleButton::textColourId,      MLIMColours::textPrimary);
    setColour (juce::ToggleButton::tickColourId,      MLIMColours::accentBlue);
    setColour (juce::ToggleButton::tickDisabledColourId, MLIMColours::textSecondary);

    // Tooltip colours
    setColour (juce::TooltipWindow::backgroundColourId, MLIMColours::widgetBackground);
    setColour (juce::TooltipWindow::textColourId,       MLIMColours::textPrimary);
    setColour (juce::TooltipWindow::outlineColourId,    MLIMColours::panelBorder);
}

void MLIMLookAndFeel::drawLinearSlider (juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPos,
                                         float /*minSliderPos*/,
                                         float /*maxSliderPos*/,
                                         juce::Slider::SliderStyle style,
                                         juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearHorizontal)
    {
        const bool isVertical = (style == juce::Slider::LinearVertical);
        const float thumbSize = 10.0f;

        // Track
        juce::Rectangle<float> trackRect;
        if (isVertical)
            trackRect = { x + width * 0.5f - 2.0f, (float)y, 4.0f, (float)height };
        else
            trackRect = { (float)x, y + height * 0.5f - 2.0f, (float)width, 4.0f };

        g.setColour (MLIMColours::panelBorder);
        g.fillRoundedRectangle (trackRect, 2.0f);

        // Filled portion
        juce::Rectangle<float> filledRect;
        if (isVertical)
            filledRect = { x + width * 0.5f - 2.0f, sliderPos, 4.0f, (float)(y + height) - sliderPos };
        else
            filledRect = { (float)x, y + height * 0.5f - 2.0f, sliderPos - x, 4.0f };

        g.setColour (MLIMColours::sliderFill);
        g.fillRoundedRectangle (filledRect, 2.0f);

        // Thumb
        if (isVertical)
        {
            g.setColour (MLIMColours::knobPointer);
            g.fillRoundedRectangle (x + width * 0.5f - thumbSize * 0.5f,
                                    sliderPos - thumbSize * 0.5f,
                                    thumbSize, thumbSize, thumbSize * 0.3f);
        }
        else
        {
            g.setColour (MLIMColours::knobPointer);
            g.fillRoundedRectangle (sliderPos - thumbSize * 0.5f,
                                    y + height * 0.5f - thumbSize * 0.5f,
                                    thumbSize, thumbSize, thumbSize * 0.3f);
        }
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                                          sliderPos, 0.0f, 0.0f, style, slider);
    }
}

void MLIMLookAndFeel::drawComboBox (juce::Graphics& g,
                                     int width, int height,
                                     bool /*isButtonDown*/,
                                     int /*buttonX*/, int /*buttonY*/,
                                     int /*buttonW*/, int /*buttonH*/,
                                     juce::ComboBox& /*box*/)
{
    const juce::Rectangle<int> bounds (0, 0, width, height);

    g.setColour (MLIMColours::displayBackground);
    g.fillRoundedRectangle (bounds.toFloat(), 3.0f);

    g.setColour (MLIMColours::panelBorder);
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 3.0f, 1.0f);

    // Arrow
    const float arrowSize = height * 0.3f;
    const float arrowX = width - arrowSize * 1.5f;
    const float arrowY = height * 0.5f - arrowSize * 0.3f;

    juce::Path arrow;
    arrow.addTriangle (arrowX, arrowY,
                       arrowX + arrowSize, arrowY,
                       arrowX + arrowSize * 0.5f, arrowY + arrowSize * 0.6f);
    g.setColour (MLIMColours::textSecondary);
    g.fillPath (arrow);
}

void MLIMLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                             juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    const juce::Rectangle<float> bounds = button.getLocalBounds().toFloat();

    // True Peak status button: transparent background, green top bar when on
    if (button.getComponentID() == "truePeakStatus")
    {
        if (button.getToggleState())
        {
            g.setColour (juce::Colour (0xff44CC44));
            g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 2.5f);
        }
        return;
    }

    // Use the button's own backgroundColour (buttonColourId when off, buttonOnColourId when on).
    // This lets individual components control their colour via setColour(buttonColourId, ...).
    juce::Colour fillColour = backgroundColour;
    if (shouldDrawButtonAsDown)
        fillColour = MLIMColours::accentBlue.darker (0.3f);
    else if (shouldDrawButtonAsHighlighted)
        fillColour = backgroundColour.brighter (0.15f);

    g.setColour (fillColour);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (MLIMColours::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}

void MLIMLookAndFeel::drawTooltip (juce::Graphics& g,
                                    const juce::String& text,
                                    int width, int height)
{
    const juce::Rectangle<int> bounds (0, 0, width, height);

    // Dark background
    g.setColour (MLIMColours::widgetBackground);
    g.fillRoundedRectangle (bounds.toFloat(), 3.0f);

    // Subtle border
    g.setColour (MLIMColours::panelBorder);
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 3.0f, 1.0f);

    // Text
    g.setColour (MLIMColours::textPrimary);
    g.setFont (juce::Font (12.0f));
    g.drawFittedText (text, bounds.reduced (4, 2), juce::Justification::centred, 1);
}
