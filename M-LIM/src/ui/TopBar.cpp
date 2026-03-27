#include "TopBar.h"
#include "Colours.h"

TopBar::TopBar()
{
    // Logo label
    logoLabel_.setText ("M-LIM", juce::dontSendNotification);
    logoLabel_.setFont (juce::Font (16.0f, juce::Font::bold));
    logoLabel_.setColour (juce::Label::textColourId, MLIMColours::textPrimary);
    logoLabel_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (logoLabel_);

    // Preset name label
    presetLabel_.setText ("Default", juce::dontSendNotification);
    presetLabel_.setFont (juce::Font (12.0f));
    presetLabel_.setColour (juce::Label::textColourId,       MLIMColours::textPrimary);
    presetLabel_.setColour (juce::Label::backgroundColourId, MLIMColours::topBarPresetBackground);
    presetLabel_.setJustificationType (juce::Justification::centred);
    presetLabel_.setColour (juce::Label::outlineColourId, MLIMColours::panelBorder);
    addAndMakeVisible (presetLabel_);

    // Prev / Next buttons
    styleBarButton (prevButton_);
    styleBarButton (nextButton_);
    prevButton_.onClick = [this] { if (onPresetPrev) onPresetPrev(); };
    nextButton_.onClick = [this] { if (onPresetNext) onPresetNext(); };
    addAndMakeVisible (prevButton_);
    addAndMakeVisible (nextButton_);

    // A/B toggle
    abToggleButton_.setClickingTogglesState (true);
    styleBarButton (abToggleButton_);
    abToggleButton_.setColour (juce::TextButton::buttonOnColourId,
                               MLIMColours::accentBlue.withAlpha (0.8f));
    abToggleButton_.onClick = [this] { if (onABToggle) onABToggle(); };
    addAndMakeVisible (abToggleButton_);

    // A→B copy
    styleBarButton (abCopyButton_);
    abCopyButton_.onClick = [this] { if (onABCopy) onABCopy(); };
    addAndMakeVisible (abCopyButton_);

    // Undo / Redo
    styleBarButton (undoButton_);
    styleBarButton (redoButton_);
    undoButton_.onClick = [this] { if (onUndo) onUndo(); };
    redoButton_.onClick = [this] { if (onRedo) onRedo(); };
    addAndMakeVisible (undoButton_);
    addAndMakeVisible (redoButton_);

    // Help button (far right, visual only)
    styleBarButton (helpButton_);
    addAndMakeVisible (helpButton_);
}

// ─────────────────────────────────────────────────────────────────────────────

void TopBar::setPresetName (const juce::String& name)
{
    presetLabel_.setText (name, juce::dontSendNotification);
}

juce::String TopBar::getPresetName() const
{
    return presetLabel_.getText();
}

// ─────────────────────────────────────────────────────────────────────────────

void TopBar::styleBarButton (juce::TextButton& btn)
{
    btn.setColour (juce::TextButton::buttonColourId,  MLIMColours::widgetBackground);
    btn.setColour (juce::TextButton::buttonOnColourId,MLIMColours::buttonPressedBackground);
    btn.setColour (juce::TextButton::textColourOffId, MLIMColours::textSecondary);
    btn.setColour (juce::TextButton::textColourOnId,  MLIMColours::textPrimary);
}

// ─────────────────────────────────────────────────────────────────────────────

void TopBar::paint (juce::Graphics& g)
{
    // Gradient background: slight depth matching Pro-L 2 top bar
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bg (juce::Colour (0xff252228), 0.0f, 0.0f,  // slight purple tint
                             juce::Colour (0xff1F1C22), 0.0f, bounds.getHeight(),
                             false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

    // Bottom separator line
    g.setColour (MLIMColours::panelBorder);
    g.drawHorizontalLine (getHeight() - 1, 0.0f, static_cast<float> (getWidth()));
}

void TopBar::resized()
{
    auto bounds = getLocalBounds().reduced (4, 2);

    static constexpr int kLogoW   = 60;
    static constexpr int kArrowW  = 20;
    static constexpr int kBtnW    = 38;
    static constexpr int kBtnWide = 44;
    static constexpr int kHelpW   = 22;
    static constexpr int kGap     = 3;

    // Left: logo
    logoLabel_.setBounds (bounds.removeFromLeft (kLogoW));
    bounds.removeFromLeft (kGap);

    // Far right: Help button
    helpButton_.setBounds (bounds.removeFromRight (kHelpW));
    bounds.removeFromRight (kGap);

    // Right section: Redo Undo A→B A/B (remove from right)
    redoButton_.setBounds    (bounds.removeFromRight (kBtnW));
    bounds.removeFromRight (kGap);
    undoButton_.setBounds    (bounds.removeFromRight (kBtnW));
    bounds.removeFromRight (kGap);
    abCopyButton_.setBounds  (bounds.removeFromRight (kBtnWide));
    bounds.removeFromRight (kGap);
    abToggleButton_.setBounds(bounds.removeFromRight (kBtnW));
    bounds.removeFromRight (kGap * 2);

    // Center: prev | preset name | next
    nextButton_.setBounds  (bounds.removeFromRight (kArrowW));
    prevButton_.setBounds  (bounds.removeFromLeft  (kArrowW));
    presetLabel_.setBounds (bounds);
}
