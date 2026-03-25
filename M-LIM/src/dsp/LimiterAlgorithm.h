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
            // Minimal coloration: slow attack, gentle knee, adaptive release; ideal for mastering
            return { 0.3f, 0.5f, 0.0f, 0.0f, 6.0f, true };

        case LimiterAlgorithm::Punchy:
            // Retains transient punch: fast attack, slight saturation and transient boost, tight knee
            return { 0.9f, 0.6f, 0.3f, 0.2f, 2.0f, false };

        case LimiterAlgorithm::Dynamic:
            // Preserves dynamics: medium attack, slower release shape, strong transient enhancement
            return { 0.5f, 0.7f, 0.2f, 0.6f, 4.0f, true };

        case LimiterAlgorithm::Aggressive:
            // Maximum loudness: very fast attack, heavy saturation, hard knee, no adaptive release
            return { 0.95f, 0.8f, 0.8f, 0.3f, 0.5f, false };

        case LimiterAlgorithm::Allround:
            // Balanced all-purpose setting: moderate attack/release, some saturation and enhancement
            return { 0.6f, 0.55f, 0.4f, 0.3f, 4.0f, true };

        case LimiterAlgorithm::Bus:
            // Bus/mix glue: medium attack, fast release, heavy saturation, wide knee, no adaptive
            return { 0.5f, 0.3f, 0.7f, 0.1f, 6.0f, false };

        case LimiterAlgorithm::Safe:
            // Conservative limiting: very slow attack/release, no saturation or enhancement, wide knee
            return { 0.2f, 0.2f, 0.0f, 0.0f, 12.0f, false };

        case LimiterAlgorithm::Modern:
            // Contemporary loudness: fast attack, light saturation, adaptive release, medium knee
            return { 0.85f, 0.5f, 0.1f, 0.2f, 3.0f, true };

        default:
            // Fallback neutral preset
            return { 0.5f, 0.5f, 0.2f, 0.1f, 4.0f, false };
    }
}
