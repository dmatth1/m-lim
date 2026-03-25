#pragma once
#include <cmath>
#include <algorithm>

static constexpr float kDspUtilMinGain = 1e-6f;  // -120 dB floor

// ---------------------------------------------------------------------------
// dB / linear conversion helpers shared across DSP translation units.
// These are thin wrappers around std::log10 / std::pow, kept as inline
// functions to avoid a JUCE header dependency inside tight DSP loops.
// ---------------------------------------------------------------------------

inline float gainToDecibels(float linearGain)
{
    return 20.0f * std::log10(std::max(linearGain, kDspUtilMinGain));
}

inline float decibelsToGain(float dB)
{
    return std::pow(10.0f, dB * (1.0f / 20.0f));
}
