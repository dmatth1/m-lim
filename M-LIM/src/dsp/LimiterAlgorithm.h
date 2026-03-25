#pragma once

static constexpr int kNumAlgorithms = 8;
static constexpr const char* kAlgorithmNames[kNumAlgorithms] = {
    "Transparent", "Punchy", "Dynamic", "Aggressive",
    "Allround", "Bus", "Safe", "Modern"
};

enum class LimiterAlgorithm
{
    Transparent = 0,
    Punchy      = 1,
    Dynamic     = 2,
    Aggressive  = 3,
    Allround    = 4,
    Bus         = 5,
    Safe        = 6,
    Modern      = 7
};

struct AlgorithmParams
{
    float transientAttackCoeff; // how aggressively transients are caught (0-1)
    float releaseShape;         // release curve shape / exponential factor (0-1)
    float saturationAmount;     // soft clipping amount (0 = none, 1 = max)
    float dynamicEnhance;       // transient enhancement before limiting (0-1)
    float kneeWidth;            // soft knee width in dB (0-12)
    bool  adaptiveRelease;      // enable adaptive release behavior
};

inline AlgorithmParams getAlgorithmParams (LimiterAlgorithm algo)
{
    switch (algo)
    {
        case LimiterAlgorithm::Transparent:
            return { 0.3f, 0.5f, 0.0f, 0.0f, 6.0f, true };

        case LimiterAlgorithm::Punchy:
            return { 0.9f, 0.6f, 0.3f, 0.2f, 2.0f, false };

        case LimiterAlgorithm::Dynamic:
            return { 0.5f, 0.7f, 0.2f, 0.6f, 4.0f, true };

        case LimiterAlgorithm::Aggressive:
            return { 0.95f, 0.8f, 0.8f, 0.3f, 0.5f, false };

        case LimiterAlgorithm::Allround:
            return { 0.6f, 0.55f, 0.4f, 0.3f, 4.0f, true };

        case LimiterAlgorithm::Bus:
            return { 0.5f, 0.3f, 0.7f, 0.1f, 6.0f, false };

        case LimiterAlgorithm::Safe:
            return { 0.2f, 0.2f, 0.0f, 0.0f, 12.0f, false };

        case LimiterAlgorithm::Modern:
            return { 0.85f, 0.5f, 0.1f, 0.2f, 3.0f, true };

        default:
            return { 0.5f, 0.5f, 0.2f, 0.1f, 4.0f, false };
    }
}
