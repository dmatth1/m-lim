#include "ControlStrip.h"

namespace
{
    // Heights for each row
    static constexpr int kKnobRowH    = 70;
    static constexpr int kBtnRowH     = 24;
    static constexpr int kPadding     = 4;
    static constexpr int kKnobLabelH  = 12;  // headroom above knob row for section labels

    // Knob column width (equal slices in the top row)
    static constexpr int kNumKnobs  = 7;   // algo(x2) + 5 knobs; inputGain is on waveform edge, outputCeiling is separate vertical slider
    // Width reserved for the output ceiling vertical slider on the far right
    static constexpr int kOutputSliderW = 40;
    // Label height above the output ceiling slider
    static constexpr int kOutputLabelH  = 14;

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

    // Advanced button width
    static constexpr int kAdvancedBtnW = 72;
}

ControlStrip::ControlStrip (juce::AudioProcessorValueTreeState& apvts)
    : apvts_ (apvts)
{
    // ── Knobs: set labels, ranges, suffixes ────────────────────────────────
    // Note: inputGain is now a waveform-overlay slider in PluginEditor.
    lookaheadKnob_.setLabel ("LOOKAHEAD");
    lookaheadKnob_.setSuffix ("ms");
    lookaheadKnob_.setRange (0.0f, 5.0f, 0.01f);

    attackKnob_.setLabel ("ATTACK");
    attackKnob_.setSuffix ("ms");
    attackKnob_.setRange (0.0f, 100.0f, 0.01f);

    releaseKnob_.setLabel ("RELEASE");
    releaseKnob_.setSuffix ("ms");
    releaseKnob_.setRange (10.0f, 1000.0f, 0.1f);

    channelLinkTransientsKnob_.setLabel ("TRANSIENTS");
    channelLinkTransientsKnob_.setSuffix ("%");
    channelLinkTransientsKnob_.setRange (0.0f, 100.0f, 0.1f);

    channelLinkReleaseKnob_.setLabel ("RELEASE");
    channelLinkReleaseKnob_.setSuffix ("%");
    channelLinkReleaseKnob_.setRange (0.0f, 100.0f, 0.1f);

    // Output Ceiling vertical slider setup
    outputCeilingSlider_.setRange (-30.0, 0.0, 0.01);
    outputCeilingSlider_.setValue (-0.1, juce::dontSendNotification);
    outputCeilingSlider_.setTextValueSuffix (" dBTP");
    outputCeilingSlider_.setNumDecimalPlacesToDisplay (2);
    outputCeilingSlider_.setColour (juce::Slider::trackColourId,      MLIMColours::panelBorder);
    outputCeilingSlider_.setColour (juce::Slider::thumbColourId,      MLIMColours::textPrimary);
    outputCeilingSlider_.setColour (juce::Slider::backgroundColourId, MLIMColours::displayBackground);
    outputCeilingSlider_.setColour (juce::Slider::textBoxTextColourId,        MLIMColours::textPrimary);
    outputCeilingSlider_.setColour (juce::Slider::textBoxBackgroundColourId,  MLIMColours::displayBackground);
    outputCeilingSlider_.setColour (juce::Slider::textBoxOutlineColourId,     MLIMColours::panelBorder);

    outputCeilingLabel_.setText ("OUTPUT", juce::dontSendNotification);
    outputCeilingLabel_.setJustificationType (juce::Justification::centred);
    outputCeilingLabel_.setFont (juce::Font (MLIMColours::kFontSizeMedium, juce::Font::bold));
    outputCeilingLabel_.setColour (juce::Label::textColourId, MLIMColours::textSecondary);

    // ── Setup buttons, combo boxes, and status bar ─────────────────────────
    setupButtons();
    setupComboBoxes();
    setupStatusBar();

    // ── Add top-row knobs ─────────────────────────────────────────────────
    addAndMakeVisible (algorithmSelector_);
    addAndMakeVisible (lookaheadKnob_);
    addAndMakeVisible (attackKnob_);
    addAndMakeVisible (releaseKnob_);
    // Channel linking knobs are always visible (Pro-L 2 parity)
    addAndMakeVisible (channelLinkTransientsKnob_);
    addAndMakeVisible (channelLinkReleaseKnob_);
    addAndMakeVisible (outputCeilingSlider_);
    addAndMakeVisible (outputCeilingLabel_);

    // ── Advanced toggle button ─────────────────────────────────────────────
    advancedButton_.setButtonText ("ADVANCED >>");
    advancedButton_.setClickingTogglesState (true);
    advancedButton_.setColour (juce::TextButton::buttonColourId,   MLIMColours::buttonBackground);
    advancedButton_.setColour (juce::TextButton::buttonOnColourId, MLIMColours::accentBlue.withAlpha (0.7f));
    advancedButton_.setColour (juce::TextButton::textColourOffId,  MLIMColours::textSecondary);
    advancedButton_.setColour (juce::TextButton::textColourOnId,   MLIMColours::textPrimary);
    addAndMakeVisible (advancedButton_);

    advancedButton_.onClick = [this]
    {
        isAdvancedExpanded_ = advancedButton_.getToggleState();
        advancedButton_.setButtonText (isAdvancedExpanded_ ? "ADVANCED <<" : "ADVANCED >>");
        resized();
    };

    // ── APVTS-bound controls (hidden — kept only for parameter attachments) ─
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
    midiLearnButton_.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);

    // ── True Peak Limiting toggle (directly APVTS-attached) ──
    truePeakLimitingButton_.setClickingTogglesState (true);
    truePeakLimitingButton_.setComponentID ("truePeakStatus");
    truePeakLimitingButton_.setColour (juce::TextButton::buttonColourId,
                                       juce::Colours::transparentBlack);
    truePeakLimitingButton_.setColour (juce::TextButton::buttonOnColourId,
                                       juce::Colours::transparentBlack);
    truePeakLimitingButton_.setColour (juce::TextButton::textColourOffId,
                                       MLIMColours::textSecondary);
    truePeakLimitingButton_.setColour (juce::TextButton::textColourOnId,
                                       MLIMColours::textPrimary);

    // ── Oversampling status label ──────────────────────────────────────────
    oversamplingStatusLabel_.setFont (juce::Font (MLIMColours::kFontSizeMedium));
    oversamplingStatusLabel_.setColour (juce::Label::textColourId, MLIMColours::textSecondary);
    oversamplingStatusLabel_.setJustificationType (juce::Justification::centredLeft);
    oversamplingStatusLabel_.setText ("Oversampling: Off", juce::dontSendNotification);

    // ── Dither status label ────────────────────────────────────────────────
    ditherStatusLabel_.setFont (juce::Font (MLIMColours::kFontSizeMedium));
    ditherStatusLabel_.setColour (juce::Label::textColourId, MLIMColours::textSecondary);
    ditherStatusLabel_.setJustificationType (juce::Justification::centredLeft);
    ditherStatusLabel_.setText ("Dither: Off", juce::dontSendNotification);

    // ── Right-section buttons ──────────────────────────────────────────────
    styleToggleButton (truePeakWaveformButton_);    // TP: show true peak on waveform
    styleStatusButton (waveformModeButton_);
    styleToggleButton (loudnessToggleButton_);
    styleStatusButton (pauseMeasurementButton_);

    measurementModeButton_.setColour (juce::TextButton::buttonColourId,
                                      MLIMColours::buttonBackground);
    measurementModeButton_.setColour (juce::TextButton::textColourOffId,
                                      MLIMColours::textSecondary);
    measurementModeButton_.onClick = [this] { cycleMeasurementMode(); };

    // ── Output level label ─────────────────────────────────────────────────
    outputLevelLabel_.setFont (juce::Font (MLIMColours::kFontSizeMedium, juce::Font::bold));
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
        cb.setColour (juce::ComboBox::backgroundColourId, MLIMColours::widgetBackground);
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
    static constexpr const char* kOversamplingLabels[] = {
        "Oversampling: Off", "Oversampling: 2x",  "Oversampling: 4x",
        "Oversampling: 8x",  "Oversampling: 16x", "Oversampling: 32x"
    };
    static constexpr int kNumOversamplingLabels = (int) std::size (kOversamplingLabels);

    const int osIdx = oversamplingBox_.getSelectedId() - 1;
    jassert (osIdx >= 0 && osIdx < kNumOversamplingLabels);
    oversamplingStatusLabel_.setText (
        kOversamplingLabels[juce::jlimit (0, kNumOversamplingLabels - 1, osIdx)],
        juce::dontSendNotification);

    // ── Dither label ───────────────────────────────────────────────────────
    const bool ditherOn = ditherButton_.getToggleState();
    if (! ditherOn)
    {
        ditherStatusLabel_.setText ("Dither: Off", juce::dontSendNotification);
        return;
    }

    static constexpr const char* kBitDepthLabels[]     = { "16", "18", "20", "22", "24" };
    static constexpr const char* kNoiseShapingChars[]  = { "B", "O", "W" };  // Basic, Optimized, Weighted
    static constexpr int kNumBitDepthLabels    = (int) std::size (kBitDepthLabels);
    static constexpr int kNumNoiseShapingChars = (int) std::size (kNoiseShapingChars);

    const int bitIdx = ditherBitDepthBox_.getSelectedId() - 1;
    const int nsIdx  = ditherNoiseShapingBox_.getSelectedId() - 1;

    jassert (bitIdx >= 0 && bitIdx < kNumBitDepthLabels);
    jassert (nsIdx  >= 0 && nsIdx  < kNumNoiseShapingChars);

    ditherStatusLabel_.setText (
        juce::String ("Dither: ")
            + kBitDepthLabels[juce::jlimit (0, kNumBitDepthLabels - 1, bitIdx)]
            + " Bits ("
            + kNoiseShapingChars[juce::jlimit (0, kNumNoiseShapingChars - 1, nsIdx)]
            + ")",
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
    // inputGainAttach_ is created in PluginEditor (waveform-edge slider)

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
        apvts_, ParamID::outputCeiling, outputCeilingSlider_);

    truePeakAttach_   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, ParamID::truePeakEnabled, truePeakLimitingButton_);

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
    btn.setColour (juce::TextButton::buttonColourId,   MLIMColours::buttonBackground);
    btn.setColour (juce::TextButton::buttonOnColourId, MLIMColours::accentBlue.withAlpha (0.85f));
    btn.setColour (juce::TextButton::textColourOffId,  MLIMColours::textSecondary);
    btn.setColour (juce::TextButton::textColourOnId,   MLIMColours::textPrimary);
}

void ControlStrip::styleStatusButton (juce::TextButton& btn)
{
    btn.setColour (juce::TextButton::buttonColourId,  MLIMColours::background);
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
    int sepY = kKnobLabelH + kKnobRowH + kPadding;
    g.drawHorizontalLine (sepY, 0.0f, static_cast<float> (getWidth()));

    // Draw "STYLE" label above the algorithm selector
    {
        auto algoB = algorithmSelector_.getBounds();
        g.setColour (MLIMColours::textSecondary);
        g.setFont (juce::Font (MLIMColours::kFontSizeSmall, juce::Font::bold));
        g.drawText ("STYLE",
                    algoB.getX(), algoB.getY() - 12,
                    algoB.getWidth(), 12,
                    juce::Justification::centred, false);
    }

    // Draw CHANNEL LINKING section overlay (always visible — Pro-L 2 parity)
    {
        auto linkBounds = channelLinkTransientsKnob_.getBounds()
                              .getUnion (channelLinkReleaseKnob_.getBounds())
                              .expanded (4, 2);
        g.setColour (MLIMColours::panelOverlay);
        g.fillRoundedRectangle (linkBounds.toFloat(), 4.0f);
        g.setColour (MLIMColours::panelBorder);
        g.drawRoundedRectangle (linkBounds.toFloat(), 4.0f, 1.0f);

        // "CHANNEL LINKING" label above the panel
        g.setColour (MLIMColours::textSecondary);
        g.setFont (juce::Font (MLIMColours::kFontSizeSmall, juce::Font::bold));
        g.drawText ("CHANNEL LINKING",
                    linkBounds.getX(), linkBounds.getY() - 12,
                    linkBounds.getWidth(), 12,
                    juce::Justification::centred, false);
    }
}

void ControlStrip::resized()
{
    auto bounds = getLocalBounds().reduced (kPadding);

    // ── Output Ceiling vertical slider: reserve right column spanning both rows ──
    const int totalH = kKnobRowH + kPadding + kBtnRowH; // full height of both rows
    auto rightCol = bounds.removeFromRight (kOutputSliderW);

    // Label at the top of the right column
    auto labelArea = rightCol.removeFromTop (kOutputLabelH);
    outputCeilingLabel_.setBounds (labelArea);

    // Slider takes remaining height of right column
    outputCeilingSlider_.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                          kOutputSliderW, 16);
    outputCeilingSlider_.setBounds (rightCol.withHeight (totalH - kOutputLabelH));

    // ── Top row: knobs ────────────────────────────────────────────────────
    bounds.removeFromTop (kKnobLabelH);  // headroom for "STYLE" and "CHANNEL LINKING" labels
    auto knobRow = bounds.removeFromTop (kKnobRowH);

    // 7 equal knob slots: 2 (algo) + 1 (lookahead) + 1 (attack) + 1 (release) + 1 (cl-transients) + 1 (cl-release)
    // ADVANCED button gets its fixed width on the right
    int knobW = (knobRow.getWidth() - kAdvancedBtnW - kPadding) / 7;
    algorithmSelector_.setBounds (knobRow.removeFromLeft (knobW * 2));

    // Basic knobs: Lookahead, Attack, Release
    lookaheadKnob_.setBounds (knobRow.removeFromLeft (knobW));
    attackKnob_.setBounds    (knobRow.removeFromLeft (knobW));
    releaseKnob_.setBounds   (knobRow.removeFromLeft (knobW));

    // Channel linking knobs: always visible (Pro-L 2 parity)
    channelLinkTransientsKnob_.setBounds (knobRow.removeFromLeft (knobW));
    channelLinkReleaseKnob_.setBounds    (knobRow.removeFromLeft (knobW));

    // ADVANCED button in remaining space
    advancedButton_.setBounds (knobRow);

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
