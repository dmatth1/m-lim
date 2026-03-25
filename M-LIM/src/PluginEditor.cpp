#include "PluginEditor.h"

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

    wireCallbacks();

    topBar_.setPresetName (audioProcessor.presetManager.getCurrentPresetName());

    setResizable (true, true);
    getConstrainer()->setMinimumSize (600, 350);
    setSize (kDefaultWidth, kDefaultHeight);

    startTimerHz (60);
}

MLIMAudioProcessorEditor::~MLIMAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
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
}

void MLIMAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void MLIMAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    topBar_.setBounds (bounds.removeFromTop (kTopBarH));
    controlStrip_.setBounds (bounds.removeFromBottom (kControlStripH));

    // Left: input level meter
    inputMeter_.setBounds (bounds.removeFromLeft (kInputMeterW));

    // Right side (right to left): loudness panel, output meter, GR meter
    loudnessPanel_.setBounds (bounds.removeFromRight (kLoudnessPanelW));
    outputMeter_.setBounds   (bounds.removeFromRight (kOutputMeterW));
    grMeter_.setBounds       (bounds.removeFromRight (kGRMeterW));

    // Centre: waveform display
    waveformDisplay_.setBounds (bounds);
}

void MLIMAudioProcessorEditor::timerCallback()
{
    auto& fifo = audioProcessor.getMeterFIFO();
    MeterData data;

    while (fifo.pop (data))
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

    // Age peak hold counters
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
