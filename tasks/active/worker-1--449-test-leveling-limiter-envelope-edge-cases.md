# Task 449: LevelingLimiter — Envelope Saturation and Adaptive Release Boundary Tests

## Description
Missing LevelingLimiter envelope boundary tests:
1. Zero release time → instant recovery to unity gain
2. Adaptive release exact 0.5 dB boundary
3. Gain never exceeds unity (no overshoot after hard limiting)
4. Rapid threshold change causes no overshoot

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LevelingLimiter.h` — release, attack params, adaptive release
Read: `src/dsp/LevelingLimiter.cpp` — envelope state update logic
Read: `tests/dsp/test_leveling_limiter.cpp` — existing tests
Modify: `tests/dsp/test_leveling_limiter.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LevelingLimiter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_zero_release_instant_recovery` — release=0ms, loud block then quiet block; gain within 3 dB of input within first 10 samples of quiet
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_adaptive_release_exact_boundary_0_5db` — drive envelope to exactly 0.5 dB above target; confirm faster release slope kicks in above threshold
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_gain_never_exceeds_unity` — 1000 loud blocks then silence; no output sample exceeds 1.0f
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_threshold_change_no_overshoot` — rapid threshold -6→-12→-6 dB during limiting; no output sample exceeds 1.0f

## Dependencies
None
