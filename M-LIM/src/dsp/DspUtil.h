#pragma once
#include <cmath>
#include <algorithm>
#include <cstring>

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

// ---------------------------------------------------------------------------
// Channel-linking blend helper.
// Blends each element of perChGain toward the minimum across all channels.
// @param perChGain  pointer to chCount gain values (modified in-place)
// @param chCount    number of channels
// @param link       blend factor 0=independent, 1=fully linked
// ---------------------------------------------------------------------------
// Clamps a linear threshold gain value to a safe [kDspUtilMinGain, 1.0f] range.
// kDspUtilMinGain (1e-6f) prevents division-by-zero in gain calculations.
inline float clampThreshold(float linear) noexcept
{
    return std::clamp(linear, kDspUtilMinGain, 1.0f);
}

// ---------------------------------------------------------------------------
// Bit-exact float comparison — avoids spurious parameter updates when an
// atomic float is written with the same value it already holds.
// ---------------------------------------------------------------------------
inline bool floatBitsEqual(float a, float b) noexcept
{
    return std::memcmp(&a, &b, sizeof(float)) == 0;
}

inline void applyChannelLinking(float* perChGain, int chCount, float link) noexcept
{
    if (chCount > 1 && link > 0.0f)
    {
        float minGain = 1.0f;
        for (int ch = 0; ch < chCount; ++ch)
            minGain = std::min(minGain, perChGain[ch]);

        for (int ch = 0; ch < chCount; ++ch)
            perChGain[ch] = perChGain[ch] * (1.0f - link) + minGain * link;
    }
}
