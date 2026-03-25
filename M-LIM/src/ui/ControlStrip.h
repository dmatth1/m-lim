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
 *  Bottom row: [TP] [Oversampling▾] [Dither▾] [DC] | [Bypass] [Unity] [Delta]
 *
 * All controls are attached to the AudioProcessorValueTreeState.
 */
class ControlStrip : public juce::Component
{
public:
    explicit ControlStrip (juce::AudioProcessorValueTreeState& apvts);
    ~ControlStrip() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

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

    // ── Bottom row: toggles & dropdowns ──────────────────────────────────────
    juce::TextButton truePeakButton_    { "TP" };
    juce::ComboBox   oversamplingBox_;
    juce::TextButton ditherButton_      { "DITHER" };
    juce::ComboBox   ditherBitDepthBox_;
    juce::ComboBox   ditherNoiseShapingBox_;
    juce::TextButton dcFilterButton_    { "DC" };
    juce::TextButton bypassButton_      { "BYPASS" };
    juce::TextButton unityButton_       { "1:1" };
    juce::TextButton deltaButton_       { "DELTA" };

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

    void setupButtons();
    void setupComboBoxes();
    void createAttachments();

    /** Style a toggle TextButton for the bottom row. */
    static void styleToggleButton (juce::TextButton& btn);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlStrip)
};
