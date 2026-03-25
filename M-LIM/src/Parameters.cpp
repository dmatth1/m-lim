#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Input gain: -12 to +36 dB, default 0
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::inputGain, 1 },
        "Input Gain",
        NormalisableRange<float>(-12.0f, 36.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Output ceiling: -30 to 0 dB, default -0.1
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::outputCeiling, 1 },
        "Output Ceiling",
        NormalisableRange<float>(-30.0f, 0.0f, 0.01f),
        -0.1f,
        AudioParameterFloatAttributes().withLabel("dBTP")
    ));

    // Algorithm: 0-7
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamID::algorithm, 1 },
        "Algorithm",
        StringArray { "Transparent", "Punchy", "Dynamic", "Aggressive", "Allround", "Bus", "Safe", "Modern" },
        0
    ));

    // Lookahead: 0-5 ms, default 1.0
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::lookahead, 1 },
        "Lookahead",
        NormalisableRange<float>(0.0f, 5.0f, 0.01f),
        1.0f,
        AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Attack: 0-100 ms, default 0
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::attack, 1 },
        "Attack",
        NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Release: 10-1000 ms, default 100
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::release, 1 },
        "Release",
        NormalisableRange<float>(10.0f, 1000.0f, 0.1f),
        100.0f,
        AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Channel link transients: 0-100%, default 75
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::channelLinkTransients, 1 },
        "Transient Link",
        NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        75.0f,
        AudioParameterFloatAttributes().withLabel("%")
    ));

    // Channel link release: 0-100%, default 100
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::channelLinkRelease, 1 },
        "Release Link",
        NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        AudioParameterFloatAttributes().withLabel("%")
    ));

    // True peak enabled: default true
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { ParamID::truePeakEnabled, 1 },
        "True Peak",
        true
    ));

    // Oversampling factor: 0-5 (Off/2x/4x/8x/16x/32x), default 0
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamID::oversamplingFactor, 1 },
        "Oversampling",
        StringArray { "Off", "2x", "4x", "8x", "16x", "32x" },
        0
    ));

    // DC filter enabled: default false
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { ParamID::dcFilterEnabled, 1 },
        "DC Filter",
        false
    ));

    // Dither enabled: default false
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { ParamID::ditherEnabled, 1 },
        "Dither",
        false
    ));

    // Dither bit depth: 16/18/20/22/24
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamID::ditherBitDepth, 1 },
        "Dither Bit Depth",
        StringArray { "16", "18", "20", "22", "24" },
        0
    ));

    // Dither noise shaping: Basic/Optimized/Weighted
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamID::ditherNoiseShaping, 1 },
        "Noise Shaping",
        StringArray { "Basic", "Optimized", "Weighted" },
        0
    ));

    // Bypass: default false
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { ParamID::bypass, 1 },
        "Bypass",
        false
    ));

    // Unity gain mode: default false
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { ParamID::unityGainMode, 1 },
        "Unity Gain",
        false
    ));

    // Sidechain HP freq: 20-2000 Hz, default 20
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::sidechainHPFreq, 1 },
        "SC HP Freq",
        NormalisableRange<float>(20.0f, 2000.0f, 0.1f, 0.3f),
        20.0f,
        AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // Sidechain LP freq: 2000-20000 Hz, default 20000
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::sidechainLPFreq, 1 },
        "SC LP Freq",
        NormalisableRange<float>(2000.0f, 20000.0f, 0.1f, 0.3f),
        20000.0f,
        AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // Sidechain tilt: -6 to +6 dB, default 0
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID { ParamID::sidechainTilt, 1 },
        "SC Tilt",
        NormalisableRange<float>(-6.0f, 6.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Delta: default false
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { ParamID::delta, 1 },
        "Delta",
        false
    ));

    // Display mode: 0-4 (Fast/Slow/SlowDown/Infinite/Off), default 0
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamID::displayMode, 1 },
        "Display Mode",
        StringArray { "Fast", "Slow", "SlowDown", "Infinite", "Off" },
        0
    ));

    // Loudness target: 0=-9 LUFS (CD), 1=-14 LUFS (Streaming), 2=-23 LUFS (EBU R128),
    //                  3=-24 LUFS (ATSC A/85), 4=Custom; default 1 (Streaming)
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamID::loudnessTarget, 1 },
        "Loudness Target",
        StringArray { "-9 LUFS (CD)", "-14 LUFS (Streaming)", "-23 LUFS (EBU R128)",
                      "-24 LUFS (ATSC A/85)", "Custom" },
        1
    ));

    return { params.begin(), params.end() };
}
