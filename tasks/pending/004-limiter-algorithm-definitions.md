# Task 004: Limiter Algorithm Definitions

## Description
Create the LimiterAlgorithm enum and AlgorithmParams struct with tuned parameter sets for all 8 algorithms. This is a data-only module that other DSP components consume.

## Produces
Implements: `AlgorithmDefinition`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/LimiterAlgorithm.h` — enum, struct, and getAlgorithmParams function
Read: `SPEC.md` — AlgorithmDefinition interface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors
- [ ] Run: `grep -c "LimiterAlgorithm::" M-LIM/src/dsp/LimiterAlgorithm.h` → Expected: at least 8 (all algorithms)

## Tests
- Unit: `tests/dsp/test_limiter_algorithm.cpp::test_all_algorithms_have_params` — verify getAlgorithmParams returns valid params for all 8 algorithms
- Unit: `tests/dsp/test_limiter_algorithm.cpp::test_algorithm_params_ranges` — verify all param values are in valid ranges (0-1 for coefficients, etc.)

## Technical Details
- Enum values: Transparent=0, Punchy=1, Dynamic=2, Aggressive=3, Allround=4, Bus=5, Safe=6, Modern=7
- AlgorithmParams fields per SPEC.md: transientAttackCoeff, releaseShape, saturationAmount, dynamicEnhance, kneeWidth, adaptiveRelease
- Tuning guidelines per algorithm:
  - Transparent: low saturation, wide knee, adaptive release on
  - Punchy: medium saturation, narrow knee, high transientAttackCoeff
  - Dynamic: high dynamicEnhance, medium saturation, adaptive release on
  - Aggressive: high saturation, very narrow knee, high transientAttackCoeff
  - Allround: balanced values for all params
  - Bus: high saturation, wide knee, slow release shape
  - Safe: zero saturation, widest knee, very smooth release
  - Modern: low saturation, medium knee, adaptive release, high transientAttackCoeff

## Dependencies
Requires task 001
