#include <catch_amalgamated.hpp>
#include "../../src/dsp/DspUtil.h"
#include <cmath>
#include <limits>

TEST_CASE("DspUtil: gainToDecibels zero dB roundtrip", "[DspUtil]")
{
    REQUIRE(gainToDecibels(1.0f) == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("DspUtil: gainToDecibels minus 120 floor", "[DspUtil]")
{
    float result = gainToDecibels(0.0f);
    float expected = 20.0f * std::log10(kDspUtilMinGain);
    REQUIRE(result == Catch::Approx(expected).margin(1e-3f));
    REQUIRE(result == Catch::Approx(-120.0f).margin(0.1f));
}

TEST_CASE("DspUtil: gainToDecibels negative input", "[DspUtil]")
{
    // Negative linear gain gets clamped to kDspUtilMinGain via std::max
    float result = gainToDecibels(-1.0f);
    float expected = gainToDecibels(0.0f); // Both clamp to kDspUtilMinGain
    REQUIRE(result == Catch::Approx(expected).margin(1e-5f));
}

TEST_CASE("DspUtil: decibelsToGain roundtrip", "[DspUtil]")
{
    for (float x : { 0.1f, 0.5f, 1.0f, 0.01f, 0.707f })
    {
        float dB = gainToDecibels(x);
        float recovered = decibelsToGain(dB);
        REQUIRE(recovered == Catch::Approx(x).epsilon(1e-4f));
    }
}

TEST_CASE("DspUtil: decibelsToGain minus infinity", "[DspUtil]")
{
    float result = decibelsToGain(-200.0f);
    REQUIRE(result < 1e-9f);
    REQUIRE(result >= 0.0f);
}

TEST_CASE("DspUtil: clampThreshold below min", "[DspUtil]")
{
    REQUIRE(clampThreshold(0.0f) == Catch::Approx(kDspUtilMinGain));
    REQUIRE(clampThreshold(-1.0f) == Catch::Approx(kDspUtilMinGain));
    REQUIRE(clampThreshold(1e-10f) == Catch::Approx(kDspUtilMinGain));
}

TEST_CASE("DspUtil: clampThreshold above max", "[DspUtil]")
{
    REQUIRE(clampThreshold(2.0f) == Catch::Approx(1.0f));
    REQUIRE(clampThreshold(100.0f) == Catch::Approx(1.0f));
}

TEST_CASE("DspUtil: clampThreshold in range", "[DspUtil]")
{
    for (float v : { 0.001f, 0.5f, 1.0f, kDspUtilMinGain })
    {
        REQUIRE(clampThreshold(v) == Catch::Approx(v));
    }
}

TEST_CASE("DspUtil: floatBitsEqual same value", "[DspUtil]")
{
    REQUIRE(floatBitsEqual(1.0f, 1.0f));
    REQUIRE(floatBitsEqual(0.0f, 0.0f));
    REQUIRE(floatBitsEqual(-3.14f, -3.14f));
}

TEST_CASE("DspUtil: floatBitsEqual positive negative zero", "[DspUtil]")
{
    REQUIRE_FALSE(floatBitsEqual(0.0f, -0.0f));
}

TEST_CASE("DspUtil: floatBitsEqual nan", "[DspUtil]")
{
    float nan1 = std::numeric_limits<float>::quiet_NaN();
    float nan2 = nan1; // same bit pattern
    REQUIRE(floatBitsEqual(nan1, nan2));
}

TEST_CASE("DspUtil: applyChannelLinking link zero no change", "[DspUtil]")
{
    float gains[] = { 0.8f, 0.6f };
    applyChannelLinking(gains, 2, 0.0f);
    REQUIRE(gains[0] == Catch::Approx(0.8f));
    REQUIRE(gains[1] == Catch::Approx(0.6f));
}

TEST_CASE("DspUtil: applyChannelLinking link one all min", "[DspUtil]")
{
    float gains[] = { 0.8f, 0.6f };
    applyChannelLinking(gains, 2, 1.0f);
    REQUIRE(gains[0] == Catch::Approx(0.6f));
    REQUIRE(gains[1] == Catch::Approx(0.6f));
}

TEST_CASE("DspUtil: applyChannelLinking single channel no change", "[DspUtil]")
{
    float gain = 0.5f;
    applyChannelLinking(&gain, 1, 1.0f);
    REQUIRE(gain == Catch::Approx(0.5f));
}

TEST_CASE("DspUtil: applyChannelLinking partial blend", "[DspUtil]")
{
    float gains[] = { 1.0f, 0.5f };
    applyChannelLinking(gains, 2, 0.5f);
    // ch0: 1.0 * 0.5 + 0.5 * 0.5 = 0.75
    // ch1: 0.5 * 0.5 + 0.5 * 0.5 = 0.50
    REQUIRE(gains[0] == Catch::Approx(0.75f));
    REQUIRE(gains[1] == Catch::Approx(0.5f));
}
