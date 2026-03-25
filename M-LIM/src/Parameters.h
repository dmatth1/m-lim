#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter ID constants
namespace ParamID
{
    inline const juce::String inputGain       = "inputGain";
    inline const juce::String outputCeiling   = "outputCeiling";
    inline const juce::String algorithm       = "algorithm";
    inline const juce::String lookahead       = "lookahead";
    inline const juce::String attack          = "attack";
    inline const juce::String release         = "release";
    inline const juce::String channelLinkTransients = "channelLinkTransients";
    inline const juce::String channelLinkRelease    = "channelLinkRelease";
    inline const juce::String truePeakEnabled  = "truePeakEnabled";
    inline const juce::String oversamplingFactor = "oversamplingFactor";
    inline const juce::String dcFilterEnabled  = "dcFilterEnabled";
    inline const juce::String ditherEnabled    = "ditherEnabled";
    inline const juce::String ditherBitDepth   = "ditherBitDepth";
    inline const juce::String ditherNoiseShaping = "ditherNoiseShaping";
    inline const juce::String bypass           = "bypass";
    inline const juce::String unityGainMode    = "unityGainMode";
    inline const juce::String sidechainHPFreq  = "sidechainHPFreq";
    inline const juce::String sidechainLPFreq  = "sidechainLPFreq";
    inline const juce::String sidechainTilt    = "sidechainTilt";
    inline const juce::String delta            = "delta";
    inline const juce::String displayMode      = "displayMode";
    inline const juce::String loudnessTarget   = "loudnessTarget";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
