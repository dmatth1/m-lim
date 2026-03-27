#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter ID constants
namespace ParamID
{
    constexpr const char* inputGain       = "inputGain";
    constexpr const char* outputCeiling   = "outputCeiling";
    constexpr const char* algorithm       = "algorithm";
    constexpr const char* lookahead       = "lookahead";
    constexpr const char* attack          = "attack";
    constexpr const char* release         = "release";
    constexpr const char* channelLinkTransients = "channelLinkTransients";
    constexpr const char* channelLinkRelease    = "channelLinkRelease";
    constexpr const char* truePeakEnabled  = "truePeakEnabled";
    constexpr const char* oversamplingFactor = "oversamplingFactor";
    constexpr const char* dcFilterEnabled  = "dcFilterEnabled";
    constexpr const char* ditherEnabled    = "ditherEnabled";
    constexpr const char* ditherBitDepth   = "ditherBitDepth";
    constexpr const char* ditherNoiseShaping = "ditherNoiseShaping";
    constexpr const char* bypass           = "bypass";
    constexpr const char* unityGainMode    = "unityGainMode";
    constexpr const char* sidechainHPFreq  = "sidechainHPFreq";
    constexpr const char* sidechainLPFreq  = "sidechainLPFreq";
    constexpr const char* sidechainTilt    = "sidechainTilt";
    constexpr const char* delta            = "delta";
    constexpr const char* displayMode      = "displayMode";
    constexpr const char* loudnessTarget   = "loudnessTarget";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
