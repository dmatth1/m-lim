#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "RotaryKnob.h"
#include "AlgorithmSelector.h"
#include "Colours.h"
#include "../Parameters.h"

/**
 * Bottom control strip containing all main controls:
 *
 *  Top row:    [InputGain] [AlgorithmSelector] [Lookahead] [Attack] [Release]
 *              [ChannelLinkTransients] [ChannelLinkRelease] [OutputCeiling]
 *
 *  Status bar: [MIDI Learn] [● True Peak Limiting] [Oversampling: Xx] [Dither: XX Bits]
 *              ... [TP] [≋] [Loudness] [||] [Short Term] [Out: X.X dBTP]
 *
 * All controls are attached to the AudioProcessorValueTreeState.
 */
class ControlStrip : public juce::Component,
                     private juce::ComboBox::Listener
{
public:
    explicit ControlStrip (juce::AudioProcessorValueTreeState& apvts);
    ~ControlStrip() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Update the "Out: X.X dBTP" readout in the status bar. */
    void setOutputLevel (float dBTPValue);

private:
    juce::AudioProcessorValueTreeState& apvts_;

    // ── Top row: knobs ────────────────────────────────────────────────────────
    RotaryKnob inputGainKnob_;
    AlgorithmSelector algorithmSelector_;
    RotaryKnob lookaheadKnob_;
    RotaryKnob attackKnob_;
    RotaryKnob releaseKnob_;
    RotaryKnob channelLinkTransientsKnob_;
    RotaryKnob channelLinkReleaseKnob_;
    RotaryKnob outputCeilingKnob_;

    // ── APVTS-bound controls (hidden but attached for parameter sync) ─────────
    juce::TextButton truePeakButton_    { "TP" };
    juce::ComboBox   oversamplingBox_;
    juce::TextButton ditherButton_      { "DITHER" };
    juce::ComboBox   ditherBitDepthBox_;
    juce::ComboBox   ditherNoiseShapingBox_;
    juce::TextButton dcFilterButton_    { "DC" };
    juce::TextButton bypassButton_      { "BYPASS" };
    juce::TextButton unityButton_       { "1:1" };
    juce::TextButton deltaButton_       { "DELTA" };

    // ── Pro-L 2 status bar: left section ─────────────────────────────────────
    juce::TextButton midiLearnButton_          { "MIDI Learn" };
    juce::TextButton truePeakLimitingButton_   { "True Peak Limiting" };
    juce::Label      oversamplingStatusLabel_;  // "Oversampling: Xx"
    juce::Label      ditherStatusLabel_;        // "Dither: XX Bits (O)"

    // ── Pro-L 2 status bar: right section ────────────────────────────────────
    juce::TextButton truePeakWaveformButton_   { "TP" };
    juce::TextButton waveformModeButton_       { juce::CharPointer_UTF8 ("\xe2\x89\x8b") }; // ≋
    juce::TextButton loudnessToggleButton_     { "Loudness" };
    juce::TextButton pauseMeasurementButton_   { "||" };
    juce::TextButton measurementModeButton_    { "Short Term" };
    juce::Label      outputLevelLabel_;         // "Out: X.X dBTP"

    // ── APVTS attachments ─────────────────────────────────────────────────────
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  inputGainAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algorithmAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  lookaheadAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  attackAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  releaseAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  channelLinkTransientsAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  channelLinkReleaseAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  outputCeilingAttach_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   truePeakAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   ditherAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ditherBitDepthAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ditherNoiseShapingAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   dcFilterAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   bypassAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   unityAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   deltaAttach_;

    // ── Measurement mode cycling ───────────────────────────────────────────────
    enum class MeasurementMode { ShortTerm, Momentary, Integrated };
    MeasurementMode measurementMode_ { MeasurementMode::ShortTerm };

    void setupButtons();
    void setupComboBoxes();
    void setupStatusBar();
    void createAttachments();
    void updateStatusLabels();
    void cycleMeasurementMode();

    /** Called when oversamplingBox_ or dither boxes change. */
    void comboBoxChanged (juce::ComboBox* comboBox) override;

    /** Style a toggle TextButton for the bottom row. */
    static void styleToggleButton (juce::TextButton& btn);

    /** Style a small status-bar TextButton (non-toggling). */
    static void styleStatusButton (juce::TextButton& btn);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlStrip)
};
