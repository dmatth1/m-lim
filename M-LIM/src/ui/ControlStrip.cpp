#include "ControlStrip.h"

namespace
{
    // Heights for each row
    static constexpr int kKnobRowH  = 70;
    static constexpr int kBtnRowH   = 24;
    static constexpr int kPadding   = 4;

    // Knob column width (equal slices in the top row)
    static constexpr int kNumKnobs  = 8;   // inputGain + 5 knobs + outputCeiling + algo (treated as wide)

    // Status bar element widths
    static constexpr int kMidiLearnW       = 76;
    static constexpr int kTruePeakLimitW   = 118;
    static constexpr int kOversamplingLblW = 110;
    static constexpr int kDitherLblW       = 130;
    static constexpr int kSmallBtnW        = 28;
    static constexpr int kLoudnessBtnW     = 60;
    static constexpr int kMeasureModeBtnW  = 76;
    static constexpr int kOutLevelLblW     = 88;
    static constexpr int kStatusGap        = 4;
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

    // ── Setup buttons, combo boxes, and status bar ─────────────────────────
    setupButtons();
    setupComboBoxes();
    setupStatusBar();

    // ── Add top-row knobs ─────────────────────────────────────────────────
    addAndMakeVisible (inputGainKnob_);
    addAndMakeVisible (algorithmSelector_);
    addAndMakeVisible (lookaheadKnob_);
    addAndMakeVisible (attackKnob_);
    addAndMakeVisible (releaseKnob_);
    addAndMakeVisible (channelLinkTransientsKnob_);
    addAndMakeVisible (channelLinkReleaseKnob_);
    addAndMakeVisible (outputCeilingKnob_);

    // ── APVTS-bound controls (hidden — kept only for parameter attachments) ─
    addChildComponent (truePeakButton_);
    addChildComponent (oversamplingBox_);
    addChildComponent (ditherButton_);
    addChildComponent (ditherBitDepthBox_);
    addChildComponent (ditherNoiseShapingBox_);
    addChildComponent (dcFilterButton_);
    addChildComponent (bypassButton_);
    addChildComponent (unityButton_);
    addChildComponent (deltaButton_);

    // ── Status bar: left section ──────────────────────────────────────────
    addAndMakeVisible (midiLearnButton_);
    addAndMakeVisible (truePeakLimitingButton_);
    addAndMakeVisible (oversamplingStatusLabel_);
    addAndMakeVisible (ditherStatusLabel_);

    // ── Status bar: right section ─────────────────────────────────────────
    addAndMakeVisible (truePeakWaveformButton_);
    addAndMakeVisible (waveformModeButton_);
    addAndMakeVisible (loudnessToggleButton_);
    addAndMakeVisible (pauseMeasurementButton_);
    addAndMakeVisible (measurementModeButton_);
    addAndMakeVisible (outputLevelLabel_);

    // ── APVTS attachments ─────────────────────────────────────────────────
    createAttachments();

    // ── Sync status labels to initial combobox values ─────────────────────
    updateStatusLabels();
}

ControlStrip::~ControlStrip()
{
    oversamplingBox_.removeListener (this);
    ditherBitDepthBox_.removeListener (this);
    ditherNoiseShapingBox_.removeListener (this);
}

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

void ControlStrip::setupStatusBar()
{
    // ── MIDI Learn button ──────────────────────────────────────────────────
    styleStatusButton (midiLearnButton_);

    // ── True Peak Limiting toggle (mirrors truePeakButton_ APVTS binding) ──
    truePeakLimitingButton_.setClickingTogglesState (true);
    truePeakLimitingButton_.setColour (juce::TextButton::buttonColourId,
                                       juce::Colour (0xff242424));
    truePeakLimitingButton_.setColour (juce::TextButton::buttonOnColourId,
                                       juce::Colour (0xff1A4A1A)); // dark green when on
    truePeakLimitingButton_.setColour (juce::TextButton::textColourOffId,
                                       MLIMColours::textSecondary);
    truePeakLimitingButton_.setColour (juce::TextButton::textColourOnId,
                                       juce::Colour (0xff66DD66)); // bright green text when on

    // Sync truePeakLimitingButton_ with truePeakButton_ (both represent the same parameter)
    truePeakLimitingButton_.onClick = [this]
    {
        truePeakButton_.setToggleState (truePeakLimitingButton_.getToggleState(),
                                        juce::sendNotification);
    };
    truePeakButton_.onClick = [this]
    {
        truePeakLimitingButton_.setToggleState (truePeakButton_.getToggleState(),
                                                juce::dontSendNotification);
    };

    // ── Oversampling status label ──────────────────────────────────────────
    oversamplingStatusLabel_.setFont (juce::Font (10.0f));
    oversamplingStatusLabel_.setColour (juce::Label::textColourId, MLIMColours::textSecondary);
    oversamplingStatusLabel_.setJustificationType (juce::Justification::centredLeft);
    oversamplingStatusLabel_.setText ("Oversampling: Off", juce::dontSendNotification);

    // ── Dither status label ────────────────────────────────────────────────
    ditherStatusLabel_.setFont (juce::Font (10.0f));
    ditherStatusLabel_.setColour (juce::Label::textColourId, MLIMColours::textSecondary);
    ditherStatusLabel_.setJustificationType (juce::Justification::centredLeft);
    ditherStatusLabel_.setText ("Dither: Off", juce::dontSendNotification);

    // ── Right-section buttons ──────────────────────────────────────────────
    styleToggleButton (truePeakWaveformButton_);    // TP: show true peak on waveform
    styleStatusButton (waveformModeButton_);
    styleToggleButton (loudnessToggleButton_);
    styleStatusButton (pauseMeasurementButton_);

    measurementModeButton_.setColour (juce::TextButton::buttonColourId,
                                      juce::Colour (0xff242424));
    measurementModeButton_.setColour (juce::TextButton::textColourOffId,
                                      MLIMColours::textSecondary);
    measurementModeButton_.onClick = [this] { cycleMeasurementMode(); };

    // ── Output level label ─────────────────────────────────────────────────
    outputLevelLabel_.setFont (juce::Font (10.0f, juce::Font::bold));
    outputLevelLabel_.setColour (juce::Label::textColourId, MLIMColours::textPrimary);
    outputLevelLabel_.setJustificationType (juce::Justification::centredRight);
    outputLevelLabel_.setText ("Out: -.-- dBTP", juce::dontSendNotification);

    // ── Register as listener on combo boxes to update status labels ────────
    oversamplingBox_.addListener (this);
    ditherBitDepthBox_.addListener (this);
    ditherNoiseShapingBox_.addListener (this);
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

void ControlStrip::comboBoxChanged (juce::ComboBox* /*comboBox*/)
{
    updateStatusLabels();
}

void ControlStrip::updateStatusLabels()
{
    // ── Oversampling label ─────────────────────────────────────────────────
    const int osId = oversamplingBox_.getSelectedId();
    juce::String osText;
    switch (osId)
    {
        case 1:  osText = "Oversampling: Off"; break;
        case 2:  osText = "Oversampling: 2x";  break;
        case 3:  osText = "Oversampling: 4x";  break;
        case 4:  osText = "Oversampling: 8x";  break;
        case 5:  osText = "Oversampling: 16x"; break;
        case 6:  osText = "Oversampling: 32x"; break;
        default: osText = "Oversampling: Off"; break;
    }
    oversamplingStatusLabel_.setText (osText, juce::dontSendNotification);

    // ── Dither label ───────────────────────────────────────────────────────
    const bool ditherOn = ditherButton_.getToggleState();
    if (! ditherOn)
    {
        ditherStatusLabel_.setText ("Dither: Off", juce::dontSendNotification);
        return;
    }

    const int bitDepthId = ditherBitDepthBox_.getSelectedId();
    const int nsId       = ditherNoiseShapingBox_.getSelectedId();

    juce::String bits;
    switch (bitDepthId)
    {
        case 1:  bits = "16"; break;
        case 2:  bits = "18"; break;
        case 3:  bits = "20"; break;
        case 4:  bits = "22"; break;
        case 5:  bits = "24"; break;
        default: bits = "16"; break;
    }

    juce::String nsChar;
    switch (nsId)
    {
        case 1:  nsChar = "B"; break;  // Basic
        case 2:  nsChar = "O"; break;  // Optimized
        case 3:  nsChar = "W"; break;  // Weighted
        default: nsChar = "O"; break;
    }

    ditherStatusLabel_.setText ("Dither: " + bits + " Bits (" + nsChar + ")",
                                juce::dontSendNotification);
}

void ControlStrip::setOutputLevel (float dBTPValue)
{
    juce::String text;
    if (dBTPValue <= -100.0f)
        text = "Out: -.-- dBTP";
    else
        text = "Out: " + juce::String (dBTPValue, 1) + " dBTP";

    outputLevelLabel_.setText (text, juce::dontSendNotification);
}

void ControlStrip::cycleMeasurementMode()
{
    switch (measurementMode_)
    {
        case MeasurementMode::ShortTerm:
            measurementMode_ = MeasurementMode::Momentary;
            measurementModeButton_.setButtonText ("Momentary");
            break;
        case MeasurementMode::Momentary:
            measurementMode_ = MeasurementMode::Integrated;
            measurementModeButton_.setButtonText ("Integrated");
            break;
        case MeasurementMode::Integrated:
            measurementMode_ = MeasurementMode::ShortTerm;
            measurementModeButton_.setButtonText ("Short Term");
            break;
    }
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

void ControlStrip::styleStatusButton (juce::TextButton& btn)
{
    btn.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff1E1E1E));
    btn.setColour (juce::TextButton::textColourOffId, MLIMColours::textSecondary);
}

// ─────────────────────────────────────────────────────────────────────────────

void ControlStrip::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Blue-gray gradient background (matches Pro-L 2's control strip aesthetic)
    juce::ColourGradient bg (MLIMColours::controlStripTop,
                             0.0f, 0.0f,
                             MLIMColours::controlStripBottom,
                             0.0f, static_cast<float> (bounds.getHeight()),
                             false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

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

    // ── Status bar row ────────────────────────────────────────────────────
    bounds.removeFromTop (kPadding); // gap after separator
    auto statusRow = bounds.removeFromTop (kBtnRowH);

    // ── Left section ──────────────────────────────────────────────────────
    midiLearnButton_.setBounds (statusRow.removeFromLeft (kMidiLearnW));
    statusRow.removeFromLeft (kStatusGap);

    truePeakLimitingButton_.setBounds (statusRow.removeFromLeft (kTruePeakLimitW));
    statusRow.removeFromLeft (kStatusGap);

    oversamplingStatusLabel_.setBounds (statusRow.removeFromLeft (kOversamplingLblW));
    statusRow.removeFromLeft (kStatusGap);

    ditherStatusLabel_.setBounds (statusRow.removeFromLeft (kDitherLblW));

    // ── Right section (right-aligned) ─────────────────────────────────────
    outputLevelLabel_.setBounds       (statusRow.removeFromRight (kOutLevelLblW));
    statusRow.removeFromRight (kStatusGap);

    measurementModeButton_.setBounds  (statusRow.removeFromRight (kMeasureModeBtnW));
    statusRow.removeFromRight (kStatusGap);

    pauseMeasurementButton_.setBounds (statusRow.removeFromRight (kSmallBtnW));
    statusRow.removeFromRight (kStatusGap);

    loudnessToggleButton_.setBounds   (statusRow.removeFromRight (kLoudnessBtnW));
    statusRow.removeFromRight (kStatusGap);

    waveformModeButton_.setBounds     (statusRow.removeFromRight (kSmallBtnW));
    statusRow.removeFromRight (kStatusGap);

    truePeakWaveformButton_.setBounds (statusRow.removeFromRight (kSmallBtnW));
}
