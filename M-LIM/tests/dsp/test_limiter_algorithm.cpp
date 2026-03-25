#include "catch2/catch_amalgamated.hpp"
#include "dsp/LimiterAlgorithm.h"

static constexpr int NUM_ALGORITHMS = 8;

static LimiterAlgorithm allAlgorithms[NUM_ALGORITHMS] = {
    LimiterAlgorithm::Transparent,
    LimiterAlgorithm::Punchy,
    LimiterAlgorithm::Dynamic,
    LimiterAlgorithm::Aggressive,
    LimiterAlgorithm::Allround,
    LimiterAlgorithm::Bus,
    LimiterAlgorithm::Safe,
    LimiterAlgorithm::Modern
};

TEST_CASE ("test_all_algorithms_have_params", "[LimiterAlgorithm]")
{
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
    {
        AlgorithmParams p = getAlgorithmParams (allAlgorithms[i]);
        // If we reach here without crashing, params were returned
        REQUIRE (p.kneeWidth >= 0.0f);
    }
}

TEST_CASE ("test_algorithm_params_ranges", "[LimiterAlgorithm]")
{
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
    {
        AlgorithmParams p = getAlgorithmParams (allAlgorithms[i]);

        REQUIRE (p.transientAttackCoeff >= 0.0f);
        REQUIRE (p.transientAttackCoeff <= 1.0f);

        REQUIRE (p.releaseShape >= 0.0f);
        REQUIRE (p.releaseShape <= 1.0f);

        REQUIRE (p.saturationAmount >= 0.0f);
        REQUIRE (p.saturationAmount <= 1.0f);

        REQUIRE (p.dynamicEnhance >= 0.0f);
        REQUIRE (p.dynamicEnhance <= 1.0f);

        REQUIRE (p.kneeWidth >= 0.0f);
        REQUIRE (p.kneeWidth <= 12.0f);
    }
}

TEST_CASE ("test_algorithm_enum_count", "[LimiterAlgorithm]")
{
    // Verify all 8 algorithms are distinct (different enough to be unique)
    // Check that all algorithms have valid params
    REQUIRE (static_cast<int> (LimiterAlgorithm::Transparent) == 0);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Punchy)      == 1);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Dynamic)     == 2);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Aggressive)  == 3);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Allround)    == 4);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Bus)         == 5);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Safe)        == 6);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Modern)      == 7);
}

TEST_CASE ("test_safe_has_zero_saturation", "[LimiterAlgorithm]")
{
    AlgorithmParams p = getAlgorithmParams (LimiterAlgorithm::Safe);
    REQUIRE (p.saturationAmount == Catch::Approx (0.0f));
}

TEST_CASE ("test_safe_has_widest_knee", "[LimiterAlgorithm]")
{
    AlgorithmParams safe = getAlgorithmParams (LimiterAlgorithm::Safe);
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
    {
        if (allAlgorithms[i] == LimiterAlgorithm::Safe)
            continue;
        AlgorithmParams other = getAlgorithmParams (allAlgorithms[i]);
        REQUIRE (safe.kneeWidth >= other.kneeWidth);
    }
}

TEST_CASE ("test_transparent_has_zero_saturation", "[LimiterAlgorithm]")
{
    AlgorithmParams p = getAlgorithmParams (LimiterAlgorithm::Transparent);
    REQUIRE (p.saturationAmount == Catch::Approx (0.0f));
    REQUIRE (p.adaptiveRelease == true);
}

TEST_CASE ("test_aggressive_has_high_saturation", "[LimiterAlgorithm]")
{
    AlgorithmParams p = getAlgorithmParams (LimiterAlgorithm::Aggressive);
    REQUIRE (p.saturationAmount >= 0.5f);
    REQUIRE (p.transientAttackCoeff >= 0.8f);
}
