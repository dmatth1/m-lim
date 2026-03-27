# Task: LevelingLimiter — Envelope Saturation and Adaptive Release Boundary Tests

## Description
LevelingLimiter uses an attack/release envelope in the linear domain. Several boundary conditions are untested:

1. **Zero release time**: When release = 0 ms (instantaneous), the gain should snap back to 1.0 on the very next sample after the loud block ends. No test verifies this; a bug here causes permanent gain reduction (the limiter never recovers).

2. **Adaptive release boundary at exactly 0.5 dB**: Adaptive release speeds up when the envelope is > 0.5 dB above the target. No test verifies behavior at exactly 0.5 dB (boundary condition). Off-by-epsilon errors here cause inconsistent release curves.

3. **Envelope underflow** (very long input >> threshold): After many seconds of hard limiting, the envelope gain value approaches near-zero. When the input drops below threshold, the recovery should still be governed by the release time constant, not snap instantaneously. Test that LRA stays correct after 30 seconds of very loud input.

4. **Multiple rapid threshold changes**: Changing the threshold from -6 dB to -12 dB and back quickly during active limiting should not cause the envelope to overshoot (gain > 1.0).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LevelingLimiter.h` — release, attack params, adaptive release
Read: `M-LIM/src/dsp/LevelingLimiter.cpp` — envelope state update logic
Read: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — existing tests
Modify: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LevelingLimiter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_zero_release_instant_recovery` — Set release = 0 ms. Feed loud block (5 samples at 2.0f), then quiet block (100 samples at 0.01f). Verify that within the first 10 samples of the quiet block, output amplitude is within 3 dB of input amplitude (full recovery).
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_adaptive_release_exact_boundary_0_5db` — Configure adaptive release on. Drive envelope to exactly 0.5 dB above target (measure GR, not just amplitude). Verify release speed matches the adaptive threshold — sample the release slope just above and just below 0.5 dB and confirm the faster slope kicks in above it.
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_gain_never_exceeds_unity` — Feed 1000 blocks of very loud sine (amplitude 4.0). Then feed silence. Verify gain never exceeds 1.0 at any sample (assert `outputSample <= 0.01f` for the first few quiet blocks, i.e., no overshoot expanding beyond ceiling).
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_threshold_change_no_overshoot` — While limiting loud signal (-6 dB threshold), change threshold to -12 dB and back to -6 dB rapidly. Verify no output sample exceeds 1.0 (no overshoot from threshold jump).

## Technical Details
- "Gain never exceeds unity" should be checked by reading the internal gain state if accessible, or by passing a 1.0f-amplitude signal and checking that no output sample exceeds 1.0f.
- For adaptive release boundary test, the adaptive threshold is 0.5 dB (GR > 0.5 dB triggers faster release); test should confirm this threshold is correct, not just that the feature exists.

## Dependencies
None
