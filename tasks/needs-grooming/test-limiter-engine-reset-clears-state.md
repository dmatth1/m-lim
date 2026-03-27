# Task: LimiterEngine::reset() — Verify All DSP State Cleared

## Description
`LimiterEngine::reset()` clears 12 internal components (TransientLimiter, LevelingLimiter,
TruePeakDetectors×4, DCFilters×2, Dithers×2, SidechainFilter, Oversamplers×2, working
buffers, and metering atomics). There is **no test** that calls `reset()` and verifies that
the engine output after reset matches a freshly-prepared engine.

This is important because a broken reset (e.g. forgetting to clear one component) would
cause state leakage between songs/sessions — a subtle bug that would only appear in DAW
use, never in unit tests that create fresh engines.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.cpp` — reset() implementation (~lines 100-144)
Read: `src/dsp/LimiterEngine.h` — all DSP component members
Modify: `tests/dsp/test_limiter_engine.cpp` — add reset tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reset_clears_gain_reduction` — process loud signal until GR > 3 dB, call reset(), process silence; getGainReduction() == 0
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reset_output_matches_fresh_engine` — process loud signal, reset, process sine; compare output to identically-prepared fresh engine processing same sine; max sample difference < 1e-6
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reset_clears_true_peak_state` — process loud signal, verify getTruePeakL() > 0; call reset(); after processing silence, getTruePeakL() should reflect only the silence block

## Technical Details
The key insight: after `reset()` + processing the same input, the engine should produce
bit-identical (or near-identical) output to a freshly `prepare()`'d engine. Any divergence
indicates a component whose state wasn't properly cleared.

## Dependencies
None
