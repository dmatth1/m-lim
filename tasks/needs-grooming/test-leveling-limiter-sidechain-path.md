# Task: LevelingLimiter — Sidechain Detection Path Tests

## Description
`LevelingLimiter::process()` accepts an optional `sidechainData` parameter (line 32 in
LevelingLimiter.h). When non-null, it uses the sidechain signal for envelope following
while applying gain reduction to the main audio. There are **no tests** that exercise
the sidechain path of LevelingLimiter directly.

The TransientLimiter has sidechain tests (`test_sidechain_with_lookahead`,
`test_sidechain_drives_gr_not_main`, `test_sidechain_silent_no_gr`), but LevelingLimiter
has none. The LimiterEngine currently passes `nullptr` for Stage 2's sidechain (line 313
in LimiterEngine.cpp), but the API exists and could be used in the future — and the
code path must work correctly if it is.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LevelingLimiter.h` — process() signature with sidechain
Read: `src/dsp/LevelingLimiter.cpp` — how sidechainData is used in the inner loop
Read: `tests/dsp/test_leveling_limiter.cpp` — existing tests
Modify: `tests/dsp/test_leveling_limiter.cpp` — add sidechain tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LevelingLimiter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_sidechain_drives_envelope_not_main` — main at 0.5, sidechain at 2.0; verify GR is applied (GR > 0 dB) even though main is below threshold
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_sidechain_silent_no_gr_applied` — main at 2.0, sidechain at 0.01; verify minimal/no GR (sidechain is quiet → no triggering)
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_null_sidechain_uses_main_for_detection` — main at 2.0, sidechain=nullptr; verify GR is applied (same behavior as no sidechain)

## Technical Details
Create a LevelingLimiter, prepare at 44100 Hz, set threshold to 1.0.
Create separate main and sidechain buffers. Pass `sidechainPtrs` as the
`sidechainData` parameter.

## Dependencies
None
