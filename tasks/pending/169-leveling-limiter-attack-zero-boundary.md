# Task 169: Add LevelingLimiter Attack=0 Boundary Test

## Description
In `LevelingLimiter::updateCoefficients()` (LevelingLimiter.cpp), when `mAttackMs == 0` the
attack coefficient is set to `0.0f`, meaning instantaneous attack (the envelope jumps immediately
to the target gain on the very first sample).  No test exercises this boundary; all existing tests
use `setAttack(0.0f)` only to avoid confounding the variable under test, but none verify the
*behaviour* of instant attack.

Additionally, the adaptive release speedup threshold (sustainedGRdB > 0.5f before speeding up,
with divisor 6.0f) has no boundary test — specifically, the transition point where `sustainedGRdB`
first exceeds 0.5 dB and speedup begins.

Tests to add in `test_leveling_limiter.cpp`:

1. **test_attack_zero_is_instantaneous** — Set `attackMs = 0` and `releaseMs` to a long value
   (e.g. 500 ms).  Feed one block with a signal above threshold.  On that very first block, GR
   must already be non-zero and the output peak must be ≤ threshold (within floating-point
   tolerance).  With a non-zero attack coefficient the output peak of the first block would
   overshoot; this test verifies it does not.

2. **test_adaptive_release_speedup_threshold** — With `adaptiveRelease = true`, drive a sustained
   loud signal to build exactly ~0.4 dB of GR (below the 0.5 dB threshold) and verify release
   is NOT accelerated. Then drive enough to build ~1.0 dB of GR (above 0.5 dB threshold) and
   verify release IS measurably faster than with `adaptiveRelease = false` at the same settings.
   This pins the 0.5 dB magic constant as a tested invariant.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LevelingLimiter.cpp:30-50` — updateCoefficients (attack/release coeff calc)
Read: `M-LIM/src/dsp/LevelingLimiter.cpp:150-190` — process inner loop, adaptive release speedup
Modify: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — add two new TEST_CASE blocks

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R test_leveling_limiter --output-on-failure` → Expected: all tests pass including the two new tests
- [ ] Run: `grep -c "test_attack_zero_is_instantaneous\|test_adaptive_release_speedup_threshold" M-LIM/tests/dsp/test_leveling_limiter.cpp` → Expected: 2

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_attack_zero_is_instantaneous` — first block already at full GR when attack=0
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_adaptive_release_speedup_threshold` — speedup boundary at ~0.5 dB sustained GR

## Technical Details
- `mAttackCoeff = 0.0f` means `envState` is set directly to `requiredGainLinear` on attack
  (one-sample convergence).
- For test 1: after `setAttack(0.0f)`, process exactly one block of amplitude 2.0; read
  `getGainReduction()` immediately — it should be ≥ 6 dB (since input is 2x above threshold 1.0).
- For test 2: look at `LevelingLimiter.cpp:176-183` — speedup only activates when
  `sustainedGRdB > 0.5f`. Build two test cases straddling 0.5 dB to pin the threshold.

## Dependencies
None
