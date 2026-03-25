# Task 196: LimiterEngine — tighten assertions and add combined-features edge-case tests

## Description
`test_limiter_engine.cpp` has two specific weaknesses plus missing edge-case scenarios:

1. **Weak peak assertion**: `test_full_chain_no_clip` uses `REQUIRE(peak <= 1.01f)` — a 1% margin
   is too permissive for a limiter. A well-implemented limiter should hold the ceiling to within
   floating-point rounding, not 1%. Tighten to `<= 1.0f + 1e-3f` (0.1%) or document why a looser
   bound is unavoidable (in which case add an INFO comment explaining the lookahead/GR path).

2. **Bypass + delta interaction** — set bypass=true and delta=true simultaneously; feed a loud
   signal and verify: (a) does not crash, (b) the output is either the dry signal or silence
   (implementation-defined, but must be deterministic across calls).

3. **Output ceiling < threshold** — set outputCeiling to -12 dBFS and inputGain to +6 dB on a
   signal that already exceeds the ceiling; verify the output peak never exceeds the ceiling.

4. **All features simultaneously** — enable oversampling (factor 1 = 2x), true peak limiting,
   DC filter, and dither together via `LimiterEngine` and process 10 blocks of a very loud signal.
   Verify: no crash, output peak ≤ ceiling, output is finite.

5. **Interleaved bypass toggle** — call `setBypass(true)` → process() → `setBypass(false)` →
   process() repeatedly across 20 block iterations. The output must always be finite and never
   exceed the ceiling when bypass is off.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/dsp/test_limiter_engine.cpp` — tighten existing assertion + add new TEST_CASEs
Read: `src/dsp/LimiterEngine.h` — full public API (setBypass, setDeltaMode, setOutputCeiling,
    setInputGain, setOversampling, setTruePeakEnabled, setDcFilterEnabled, setDitherEnabled)

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all limiter engine tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_full_chain_no_clip` — tighten `<= 1.01f` to `<= 1.001f`; add INFO comment if lookahead makes 1.001 infeasible
- Unit: `tests/dsp/test_limiter_engine.cpp::test_bypass_and_delta_simultaneous` — no crash; output is finite and deterministic
- Unit: `tests/dsp/test_limiter_engine.cpp::test_ceiling_below_zero_db` — -12 dBFS ceiling strictly enforced with loud input
- Unit: `tests/dsp/test_limiter_engine.cpp::test_all_features_simultaneously` — all DSP features on, no crash, output peak ≤ ceiling
- Unit: `tests/dsp/test_limiter_engine.cpp::test_interleaved_bypass_toggle` — 20-block bypass on/off cycle always produces finite output, off-blocks never exceed ceiling

## Technical Details
- `LimiterEngine` likely exposes `setBypass(bool)` and `setDeltaMode(bool)`; check the header.
- For `test_ceiling_below_zero_db`: linear ceiling = `std::pow(10.0f, -12.0f / 20.0f)` ≈ 0.251.
- For the `test_all_features_simultaneously` test, process 10 blocks of amplitude-10 sine; each block
  must satisfy `maxAbsValue(buf) <= ceiling + 1e-3f`.
- The existing `makeSine()` and `maxAbsValue()` helpers in the file are reusable.

## Dependencies
None
