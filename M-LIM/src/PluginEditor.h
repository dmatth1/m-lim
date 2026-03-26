#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/TopBar.h"
#include "ui/WaveformDisplay.h"
#include "ui/LevelMeter.h"
#include "ui/GainReductionMeter.h"
#include "ui/LoudnessPanel.h"
#include "ui/ControlStrip.h"
#include "ui/Colours.h"

class MLIMAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::Timer,
                                  public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit MLIMAudioProcessorEditor (MLIMAudioProcessor& p);
    ~MLIMAudioProcessorEditor() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;
    void timerCallback () override;
    void parameterChanged (const juce::String& paramID, float newValue) override;

    /** For testing: returns a reference to the loudness panel. */
    LoudnessPanel& getLoudnessPanel() noexcept { return loudnessPanel_; }

    /** For testing: returns a reference to the waveform display. */
    WaveformDisplay& getWaveformDisplay() noexcept { return waveformDisplay_; }

private:
    MLIMAudioProcessor& audioProcessor;

    MLIMLookAndFeel lookAndFeel_;

    TopBar              topBar_;
    WaveformDisplay     waveformDisplay_;
    LevelMeter          inputMeter_;
    LevelMeter          outputMeter_;
    GainReductionMeter  grMeter_;
    LoudnessPanel       loudnessPanel_;
    ControlStrip        controlStrip_;

    // ── Input Gain: vertical slider overlaid on waveform left edge ────────────
    juce::Slider inputGainSlider_;
    juce::Label  inputGainLabel_;       // "GAIN" text above slider
    juce::Label  inputGainValueLabel_;  // current dB value in gold
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttach_;

    // Peak hold state (2-second hold, then fall)
    float inputPeakL_  = -96.0f;
    float inputPeakR_  = -96.0f;
    float outputPeakL_ = -96.0f;
    float outputPeakR_ = -96.0f;
    int   inputPeakHoldFramesL_  = 0;
    int   inputPeakHoldFramesR_  = 0;
    int   outputPeakHoldFramesL_ = 0;
    int   outputPeakHoldFramesR_ = 0;

    static constexpr int kDefaultWidth   = 900;
    static constexpr int kDefaultHeight  = 500;
    static constexpr int kTopBarH        = 30;
    static constexpr int kControlStripH  = 120;
    static constexpr int kInputMeterW    = 48;
    static constexpr int kGRMeterW       = 40;
    static constexpr int kOutputMeterW   = 48;
    static constexpr int kLoudnessPanelW = 140;
    static constexpr int kPeakHoldFrames = 120; // ~2s at 60fps
    static constexpr int kGainSliderW    = 20;  // waveform-edge input gain slider width
    static constexpr int kGainLabelH     = 14;  // "GAIN" label height
    static constexpr int kGainValueH     = 12;  // value readout height

    void wireCallbacks();
    void updatePeakHold (float newL, float newR,
                         float& peakL, float& peakR,
                         int& framesL, int& framesR) noexcept;
    void applyMeterData (const MeterData& data);
    void agePeakHoldCounters() noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLIMAudioProcessorEditor)
};
