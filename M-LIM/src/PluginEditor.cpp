#include "PluginEditor.h"
#include "Parameters.h"
#include "ui/LoudnessPanel.h"

namespace
{
    static float linToDB (float lin) noexcept
    {
        return lin > 0.0f ? juce::Decibels::gainToDecibels (lin) : -96.0f;
    }
}

MLIMAudioProcessorEditor::MLIMAudioProcessorEditor (MLIMAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      controlStrip_ (p.apvts)
{
    setLookAndFeel (&lookAndFeel_);

    addAndMakeVisible (topBar_);
    addAndMakeVisible (waveformDisplay_);
    addAndMakeVisible (inputMeter_);
    addAndMakeVisible (outputMeter_);
    addAndMakeVisible (grMeter_);
    addAndMakeVisible (loudnessPanel_);
    addAndMakeVisible (controlStrip_);

    // ── Input Gain slider (waveform left-edge overlay) ────────────────────────
    inputGainSlider_.setSliderStyle (juce::Slider::LinearVertical);
    inputGainSlider_.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    inputGainSlider_.setRange (-12.0, 36.0, 0.01);
    inputGainSlider_.setColour (juce::Slider::backgroundColourId, juce::Colour (0x30FFFFFF));
    inputGainSlider_.setColour (juce::Slider::trackColourId,      juce::Colour (0x50FFFFFF));
    inputGainSlider_.setColour (juce::Slider::thumbColourId,      juce::Colour (0xC0FFFFFF));
    addAndMakeVisible (inputGainSlider_);

    inputGainLabel_.setText ("GAIN", juce::dontSendNotification);
    inputGainLabel_.setFont (juce::Font (9.0f, juce::Font::bold));
    inputGainLabel_.setJustificationType (juce::Justification::centred);
    inputGainLabel_.setColour (juce::Label::textColourId, MLIMColours::textSecondary);
    addAndMakeVisible (inputGainLabel_);

    inputGainValueLabel_.setText ("+0.0", juce::dontSendNotification);
    inputGainValueLabel_.setFont (juce::Font (9.0f, juce::Font::bold));
    inputGainValueLabel_.setJustificationType (juce::Justification::centred);
    inputGainValueLabel_.setColour (juce::Label::textColourId, juce::Colour (0xffFFD700));
    addAndMakeVisible (inputGainValueLabel_);

    // Keep value label in sync with slider
    inputGainSlider_.onValueChange = [this]
    {
        const double v = inputGainSlider_.getValue();
        const juce::String text = (v >= 0.0 ? "+" : "") + juce::String (v, 1);
        inputGainValueLabel_.setText (text, juce::dontSendNotification);
    };

    // APVTS attachment (fires onValueChange on construction to sync value label)
    inputGainAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, ParamID::inputGain, inputGainSlider_);

    wireCallbacks();

    // Register APVTS listeners so state restores sync UI components
    audioProcessor.apvts.addParameterListener (ParamID::loudnessTarget, this);
    audioProcessor.apvts.addParameterListener (ParamID::displayMode,    this);

    // Initial sync from APVTS
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (
            audioProcessor.apvts.getParameter (ParamID::loudnessTarget)))
        loudnessPanel_.setTargetChoice (param->getIndex());

    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (
            audioProcessor.apvts.getParameter (ParamID::displayMode)))
        waveformDisplay_.setDisplayMode (
            static_cast<WaveformDisplay::DisplayMode> (param->getIndex()));

    topBar_.setPresetName (audioProcessor.presetManager.getCurrentPresetName());

    setResizable (true, true);
    getConstrainer()->setMinimumSize (600, 350);
    setSize (kDefaultWidth, kDefaultHeight);

    startTimerHz (60);
}

MLIMAudioProcessorEditor::~MLIMAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener (ParamID::loudnessTarget, this);
    audioProcessor.apvts.removeParameterListener (ParamID::displayMode,    this);
    stopTimer();
    setLookAndFeel (nullptr);
}

void MLIMAudioProcessorEditor::parameterChanged (const juce::String& paramID, float newValue)
{
    if (paramID == ParamID::loudnessTarget)
        loudnessPanel_.setTargetChoice (static_cast<int> (newValue));
    else if (paramID == ParamID::displayMode)
        waveformDisplay_.setDisplayMode (
            static_cast<WaveformDisplay::DisplayMode> (static_cast<int> (newValue)));
}

void MLIMAudioProcessorEditor::wireCallbacks()
{
    topBar_.onUndo = [this]
    {
        audioProcessor.undoManager.undo();
    };

    topBar_.onRedo = [this]
    {
        audioProcessor.undoManager.redo();
    };

    topBar_.onABToggle = [this]
    {
        audioProcessor.abState.toggle (audioProcessor.apvts);
    };

    topBar_.onABCopy = [this]
    {
        audioProcessor.abState.copyAtoB();
    };

    topBar_.onPresetPrev = [this]
    {
        audioProcessor.presetManager.loadPreviousPreset (audioProcessor.apvts);
        topBar_.setPresetName (audioProcessor.presetManager.getCurrentPresetName());
    };

    topBar_.onPresetNext = [this]
    {
        audioProcessor.presetManager.loadNextPreset (audioProcessor.apvts);
        topBar_.setPresetName (audioProcessor.presetManager.getCurrentPresetName());
    };

    waveformDisplay_.onDisplayModeChanged = [this] (WaveformDisplay::DisplayMode mode)
    {
        if (auto* p = audioProcessor.apvts.getParameter (ParamID::displayMode))
            p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (mode)));
    };

    loudnessPanel_.onTargetChanged = [this] (int choiceIndex)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (
                audioProcessor.apvts.getParameter (ParamID::loudnessTarget)))
        {
            *param = choiceIndex;
        }
    };
}

void MLIMAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void MLIMAudioProcessorEditor::resized()
{
    // Pro-L 2 layout (left to right): input meter | waveform | GR meter | output meter | loudness panel
    auto bounds = getLocalBounds();

    // Full-width strips at top and bottom
    topBar_.setBounds       (bounds.removeFromTop    (kTopBarH));
    controlStrip_.setBounds (bounds.removeFromBottom (kControlStripH));

    // Input level meter on the LEFT edge
    inputMeter_.setBounds (bounds.removeFromLeft (kInputMeterW));

    // Right edge: loudness panel (outermost), then output meter, then GR meter adjacent to waveform
    loudnessPanel_.setBounds (bounds.removeFromRight (kLoudnessPanelW));
    outputMeter_.setBounds   (bounds.removeFromRight (kOutputMeterW));
    grMeter_.setBounds       (bounds.removeFromRight (kGRMeterW));

    // Waveform display fills the remaining centre (~70-75% of total width)
    waveformDisplay_.setBounds (bounds);

    // Input Gain slider overlaid on the left edge of the waveform area
    // "GAIN" label above, value label at bottom, slider in between
    auto gainOverlay = bounds.withWidth (kGainSliderW);
    inputGainLabel_.setBounds      (gainOverlay.removeFromTop    (kGainLabelH));
    inputGainValueLabel_.setBounds (gainOverlay.removeFromBottom (kGainValueH));
    inputGainSlider_.setBounds     (gainOverlay);
}

void MLIMAudioProcessorEditor::timerCallback()
{
    MeterData data;
    while (audioProcessor.getMeterFIFO().pop (data))
        applyMeterData (data);
    agePeakHoldCounters();
}

void MLIMAudioProcessorEditor::applyMeterData (const MeterData& data)
{
    waveformDisplay_.pushMeterData (data);

    const float inL  = linToDB (data.inputLevelL);
    const float inR  = linToDB (data.inputLevelR);
    const float outL = linToDB (data.outputLevelL);
    const float outR = linToDB (data.outputLevelR);

    inputMeter_.setLevel  (inL, inR);
    outputMeter_.setLevel (outL, outR);

    updatePeakHold (inL, inR, inputPeakL_, inputPeakR_,
                    inputPeakHoldFramesL_, inputPeakHoldFramesR_);
    updatePeakHold (outL, outR, outputPeakL_, outputPeakR_,
                    outputPeakHoldFramesL_, outputPeakHoldFramesR_);

    inputMeter_.setPeakHold  (inputPeakL_, inputPeakR_);
    outputMeter_.setPeakHold (outputPeakL_, outputPeakR_);

    // gainReduction in MeterData is negative dB (0 = no GR, -3 = 3 dB reduction)
    const float grPositive = -data.gainReduction;
    grMeter_.setGainReduction (grPositive > 0.0f ? grPositive : 0.0f);

    inputMeter_.setClip  (data.inputLevelL  >= 1.0f, data.inputLevelR  >= 1.0f);
    outputMeter_.setClip (data.outputLevelL >= 1.0f, data.outputLevelR >= 1.0f);

    loudnessPanel_.setMomentary     (data.momentaryLUFS);
    loudnessPanel_.setShortTerm     (data.shortTermLUFS);
    loudnessPanel_.setIntegrated    (data.integratedLUFS);
    loudnessPanel_.setLoudnessRange (data.loudnessRange);
    loudnessPanel_.setTruePeak      (linToDB (juce::jmax (data.truePeakL, data.truePeakR)));
}

void MLIMAudioProcessorEditor::agePeakHoldCounters() noexcept
{
    auto advancePeakHold = [](float& peak, int& frames) noexcept
    {
        if (frames > 0 && --frames == 0)
            peak = -96.0f;
    };
    advancePeakHold (inputPeakL_,  inputPeakHoldFramesL_);
    advancePeakHold (inputPeakR_,  inputPeakHoldFramesR_);
    advancePeakHold (outputPeakL_, outputPeakHoldFramesL_);
    advancePeakHold (outputPeakR_, outputPeakHoldFramesR_);
}

void MLIMAudioProcessorEditor::updatePeakHold (float newL, float newR,
                                                float& peakL, float& peakR,
                                                int& framesL, int& framesR) noexcept
{
    if (newL >= peakL)
    {
        peakL   = newL;
        framesL = kPeakHoldFrames;
    }
    if (newR >= peakR)
    {
        peakR   = newR;
        framesR = kPeakHoldFrames;
    }
}
