# Task 012: Leveling Limiter (Stage 2)

## Description
Implement the slow leveling limiter (Stage 2 of the dual-stage design). Controls the release envelope shape and provides gentle level control after the transient stage.

## Produces
Implements: `LevelingLimiterInterface`

## Consumes
AlgorithmDefinition

## Relevant Files
Create: `M-LIM/src/dsp/LevelingLimiter.h` — class declaration
Create: `M-LIM/src/dsp/LevelingLimiter.cpp` — implementation
Create: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — unit tests
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — AlgorithmParams struct
Read: `SPEC.md` — LevelingLimiterInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_leveling_limiter --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_release_envelope` — after gain reduction, level returns to unity following release time
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_attack_delays_reduction` — with slow attack, GR doesn't engage immediately
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_channel_linking` — 100% link = same GR both channels
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_adaptive_release` — when enabled, release time adapts to signal dynamics

## Technical Details
- Envelope follower with configurable attack and release times
- Attack range: 0-100ms (exponential coefficients)
- Release range: 10-1000ms (exponential coefficients)
- Coefficient calculation: coeff = exp(-1.0 / (time_ms * 0.001 * sampleRate))
- Adaptive release: when AlgorithmParams.adaptiveRelease is true, release time shortens for sustained loud sections
- Release shape controlled by AlgorithmParams.releaseShape (1.0 = linear, >1 = exponential, <1 = logarithmic)
- Channel linking: same approach as TransientLimiter
- Gain applied in linear domain
- Must be real-time safe
- **Sidechain input**: per SPEC.md, process() takes an optional `const float* const* sidechainData` parameter. When non-null, use sidechainData for envelope following while applying gain reduction to the main channelData. When null, follow channelData directly.

## Dependencies
Requires tasks 001, 004
