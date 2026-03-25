#include "ControlStrip.h"

namespace
{
    // Heights for each row
    static constexpr int kKnobRowH  = 70;
    static constexpr int kBtnRowH   = 28;
    static constexpr int kPadding   = 4;

    // Knob column width (equal slices in the top row)
    static constexpr int kNumKnobs  = 8;   // inputGain + 5 knobs + outputCeiling + algo (treated as wide)
}

ControlStrip::ControlStrip (juce::AudioProcessorValueTreeState& apvts)
    : apvts_ (apvts)
{
    // ── Knobs: set labels, ranges, suffixes ────────────────────────────────
    inputGainKnob_.setLabel ("Input");
    inputGainKnob_.setSuffix ("dB");
    inputGainKnob_.setRange (-12.0f, 36.0f, 0.01f);

    lookaheadKnob_.setLabel ("Lookahead");
    lookaheadKnob_.setSuffix ("ms");
    lookaheadKnob_.setRange (0.0f, 5.0f, 0.01f);

    attackKnob_.setLabel ("Attack");
    attackKnob_.setSuffix ("ms");
    attackKnob_.setRange (0.0f, 100.0f, 0.01f);

    releaseKnob_.setLabel ("Release");
    releaseKnob_.setSuffix ("ms");
    releaseKnob_.setRange (10.0f, 1000.0f, 0.1f);

    channelLinkTransientsKnob_.setLabel ("Tr Link");
    channelLinkTransientsKnob_.setSuffix ("%");
    channelLinkTransientsKnob_.setRange (0.0f, 100.0f, 0.1f);

    channelLinkReleaseKnob_.setLabel ("Rel Link");
    channelLinkReleaseKnob_.setSuffix ("%");
    channelLinkReleaseKnob_.setRange (0.0f, 100.0f, 0.1f);

    outputCeilingKnob_.setLabel ("Ceiling");
    outputCeilingKnob_.setSuffix ("dBTP");
    outputCeilingKnob_.setRange (-30.0f, 0.0f, 0.01f);

    // ── Setup buttons and combo boxes ──────────────────────────────────────
    setupButtons();
    setupComboBoxes();

    // ── Add children ──────────────────────────────────────────────────────
    addAndMakeVisible (inputGainKnob_);
    addAndMakeVisible (algorithmSelector_);
    addAndMakeVisible (lookaheadKnob_);
    addAndMakeVisible (attackKnob_);
    addAndMakeVisible (releaseKnob_);
    addAndMakeVisible (channelLinkTransientsKnob_);
    addAndMakeVisible (channelLinkReleaseKnob_);
    addAndMakeVisible (outputCeilingKnob_);

    addAndMakeVisible (truePeakButton_);
    addAndMakeVisible (oversamplingBox_);
    addAndMakeVisible (ditherButton_);
    addAndMakeVisible (ditherBitDepthBox_);
    addAndMakeVisible (ditherNoiseShapingBox_);
    addAndMakeVisible (dcFilterButton_);
    addAndMakeVisible (bypassButton_);
    addAndMakeVisible (unityButton_);
    addAndMakeVisible (deltaButton_);

    // ── APVTS attachments ─────────────────────────────────────────────────
    createAttachments();
}

ControlStrip::~ControlStrip() = default;

// ─────────────────────────────────────────────────────────────────────────────

void ControlStrip::setupButtons()
{
    styleToggleButton (truePeakButton_);
    styleToggleButton (ditherButton_);
    styleToggleButton (dcFilterButton_);
    styleToggleButton (bypassButton_);
    styleToggleButton (unityButton_);
    styleToggleButton (deltaButton_);
}

void ControlStrip::setupComboBoxes()
{
    // Oversampling
    oversamplingBox_.addItem ("Off",  1);
    oversamplingBox_.addItem ("2x",   2);
    oversamplingBox_.addItem ("4x",   3);
    oversamplingBox_.addItem ("8x",   4);
    oversamplingBox_.addItem ("16x",  5);
    oversamplingBox_.addItem ("32x",  6);
    oversamplingBox_.setSelectedId (1, juce::dontSendNotification);
    oversamplingBox_.setJustificationType (juce::Justification::centred);

    // Dither bit depth
    ditherBitDepthBox_.addItem ("16",  1);
    ditherBitDepthBox_.addItem ("18",  2);
    ditherBitDepthBox_.addItem ("20",  3);
    ditherBitDepthBox_.addItem ("22",  4);
    ditherBitDepthBox_.addItem ("24",  5);
    ditherBitDepthBox_.setSelectedId (1, juce::dontSendNotification);
    ditherBitDepthBox_.setJustificationType (juce::Justification::centred);

    // Dither noise shaping
    ditherNoiseShapingBox_.addItem ("Basic",     1);
    ditherNoiseShapingBox_.addItem ("Optimized", 2);
    ditherNoiseShapingBox_.addItem ("Weighted",  3);
    ditherNoiseShapingBox_.setSelectedId (1, juce::dontSendNotification);
    ditherNoiseShapingBox_.setJustificationType (juce::Justification::centred);

    // Style combo boxes
    auto styleBox = [] (juce::ComboBox& cb)
    {
        cb.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A2A));
        cb.setColour (juce::ComboBox::textColourId,       MLIMColours::textPrimary);
        cb.setColour (juce::ComboBox::outlineColourId,    MLIMColours::panelBorder);
        cb.setColour (juce::ComboBox::arrowColourId,      MLIMColours::textSecondary);
    };
    styleBox (oversamplingBox_);
    styleBox (ditherBitDepthBox_);
    styleBox (ditherNoiseShapingBox_);
}

void ControlStrip::createAttachments()
{
    inputGainAttach_  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, ParamID::inputGain, inputGainKnob_.getSlider());

    algorithmAttach_  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, ParamID::algorithm, algorithmSelector_.getComboBox());

    lookaheadAttach_  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, ParamID::lookahead, lookaheadKnob_.getSlider());

    attackAttach_     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, ParamID::attack, attackKnob_.getSlider());

    releaseAttach_    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, ParamID::release, releaseKnob_.getSlider());

    channelLinkTransientsAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, ParamID::channelLinkTransients, channelLinkTransientsKnob_.getSlider());

    channelLinkReleaseAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, ParamID::channelLinkRelease, channelLinkReleaseKnob_.getSlider());

    outputCeilingAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, ParamID::outputCeiling, outputCeilingKnob_.getSlider());

    truePeakAttach_   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, ParamID::truePeakEnabled, truePeakButton_);

    oversamplingAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, ParamID::oversamplingFactor, oversamplingBox_);

    ditherAttach_     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, ParamID::ditherEnabled, ditherButton_);

    ditherBitDepthAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, ParamID::ditherBitDepth, ditherBitDepthBox_);

    ditherNoiseShapingAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, ParamID::ditherNoiseShaping, ditherNoiseShapingBox_);

    dcFilterAttach_   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, ParamID::dcFilterEnabled, dcFilterButton_);

    bypassAttach_     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, ParamID::bypass, bypassButton_);

    unityAttach_      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, ParamID::unityGainMode, unityButton_);

    deltaAttach_      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, ParamID::delta, deltaButton_);
}

// ─────────────────────────────────────────────────────────────────────────────

void ControlStrip::styleToggleButton (juce::TextButton& btn)
{
    btn.setClickingTogglesState (true);
    btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff242424));
    btn.setColour (juce::TextButton::buttonOnColourId, MLIMColours::accentBlue.withAlpha (0.85f));
    btn.setColour (juce::TextButton::textColourOffId,  MLIMColours::textSecondary);
    btn.setColour (juce::TextButton::textColourOnId,   MLIMColours::textPrimary);
}

// ─────────────────────────────────────────────────────────────────────────────

void ControlStrip::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1A1A1A));

    // Separator line between knob row and button row
    g.setColour (MLIMColours::panelBorder);
    int sepY = kKnobRowH + kPadding;
    g.drawHorizontalLine (sepY, 0.0f, static_cast<float> (getWidth()));
}

void ControlStrip::resized()
{
    auto bounds = getLocalBounds().reduced (kPadding);

    // ── Top row: knobs ────────────────────────────────────────────────────
    auto knobRow = bounds.removeFromTop (kKnobRowH);

    // InputGain on far left
    int knobW = knobRow.getWidth() / (kNumKnobs + 1); // +1 for wider algo selector
    inputGainKnob_.setBounds (knobRow.removeFromLeft (knobW));

    // AlgorithmSelector takes 2 knob-widths
    algorithmSelector_.setBounds (knobRow.removeFromLeft (knobW * 2));

    // Remaining 5 knobs
    lookaheadKnob_.setBounds             (knobRow.removeFromLeft (knobW));
    attackKnob_.setBounds                (knobRow.removeFromLeft (knobW));
    releaseKnob_.setBounds               (knobRow.removeFromLeft (knobW));
    channelLinkTransientsKnob_.setBounds (knobRow.removeFromLeft (knobW));
    channelLinkReleaseKnob_.setBounds    (knobRow.removeFromLeft (knobW));

    // OutputCeiling on far right — takes remaining space
    outputCeilingKnob_.setBounds (knobRow);

    // ── Bottom row: toggles & dropdowns ──────────────────────────────────
    bounds.removeFromTop (kPadding); // gap after separator
    auto btnRow = bounds.removeFromTop (kBtnRowH);

    // Widths for bottom row items
    const int btnW   = 52;
    const int boxW   = 62;
    const int boxWNS = 80; // noise shaping box is wider
    const int gap    = 4;

    truePeakButton_.setBounds    (btnRow.removeFromLeft (btnW));
    btnRow.removeFromLeft (gap);

    oversamplingBox_.setBounds   (btnRow.removeFromLeft (boxW));
    btnRow.removeFromLeft (gap);

    ditherButton_.setBounds      (btnRow.removeFromLeft (btnW));
    btnRow.removeFromLeft (2);
    ditherBitDepthBox_.setBounds (btnRow.removeFromLeft (50));
    btnRow.removeFromLeft (2);
    ditherNoiseShapingBox_.setBounds (btnRow.removeFromLeft (boxWNS));
    btnRow.removeFromLeft (gap);

    dcFilterButton_.setBounds    (btnRow.removeFromLeft (btnW));

    // Right-aligned section: Bypass, Unity, Delta
    deltaButton_.setBounds  (btnRow.removeFromRight (btnW));
    btnRow.removeFromRight (gap);
    unityButton_.setBounds  (btnRow.removeFromRight (btnW));
    btnRow.removeFromRight (gap);
    bypassButton_.setBounds (btnRow.removeFromRight (btnW + 10));
}
