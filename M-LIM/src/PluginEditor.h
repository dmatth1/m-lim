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

struct PeakHoldState
{
    float peakL = -96.0f, peakR = -96.0f;
    int   framesL = 0,    framesR = 0;

    void update (float newL, float newR, int holdFrames) noexcept
    {
        if (newL >= peakL) { peakL = newL; framesL = holdFrames; }
        if (newR >= peakR) { peakR = newR; framesR = holdFrames; }
    }

    void age() noexcept
    {
        if (framesL > 0 && --framesL == 0) peakL = -96.0f;
        if (framesR > 0 && --framesR == 0) peakR = -96.0f;
    }
};

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

    // ── Input Gain: horizontal badge slider in bottom-left of waveform ───────
    juce::Slider inputGainSlider_;
    juce::Label  inputGainValueLabel_;  // badge text showing current dB value
    juce::Label  gainLabel_;            // static "GAIN" label below the badge
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttach_;

    // Peak hold state (2-second hold, then fall)
    PeakHoldState inputPeak_;
    PeakHoldState outputPeak_;

    static constexpr int kDefaultWidth   = 900;
    static constexpr int kDefaultHeight  = 500;
    static constexpr int kTopBarH        = 40;  // increased from 24 to reduce waveform-top RMSE
    static constexpr int kControlStripH  = 92;
    static constexpr int kInputMeterW    = 30;
    static constexpr int kGRMeterW       = 12;
    static constexpr int kOutputMeterW   = 100;
    static constexpr int kLoudnessPanelW = 140;
    static constexpr int kPeakHoldFrames = 120; // ~2s at 60fps
    static constexpr int kGainBadgeW     = 80;  // input gain badge width
    static constexpr int kGainBadgeH     = 30;  // input gain badge height (label + value)

    void wireCallbacks();
    void applyMeterData (const MeterData& data);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLIMAudioProcessorEditor)
};
