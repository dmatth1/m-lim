# Task 515: LimiterEngine Simultaneous Mode Combination Tests

## Description
`LimiterEngine` exposes three independent boolean mode flags: `setBypass()`, `setDeltaMode()`,
and `setUnityGain()`. The existing `test_limiter_engine_modes.cpp` tests each flag in
isolation but never verifies the interaction when two or three flags are active at once.

The `process()` implementation early-returns on bypass (line 223 of LimiterEngine.cpp)
**before** executing delta mode logic (line 247). This means:
- `bypass=true + delta=true` → bypass wins, delta never runs. Is this intentional?
- `delta=true + unity=true` → both paths execute; delta subtracts pre/post-limit, then
  unity applies ceiling compensation. Do they compose correctly?
- `bypass=true + unity=true` → bypass wins, unity is ignored.

Without explicit tests these are silent assumptions that may break silently during refactor.

Add tests for:
1. **bypass + delta**: verify that bypass takes priority (output ≈ input, not delta diff).
2. **bypass + unity**: verify bypass still passes signal unchanged (unity ignored).
3. **delta + unity**: verify output = pre_limit - post_ceiling (delta computed after unity
   ceiling is applied, not before); values should be finite and bounded.
4. **all three simultaneously**: bypass takes priority; output ≈ input regardless of the
   other flags.
5. **mode switching mid-stream**: toggle from delta→bypass→delta across three consecutive
   blocks; verify no discontinuity > 6 dB between adjacent blocks.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/tests/dsp/test_limiter_engine_modes.cpp` — append new TEST_CASE blocks
Read: `M-LIM/src/dsp/LimiterEngine.h` — understand setBypass/setDeltaMode/setUnityGain
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — lines 223-250 (process() flow) and lines 280-400 (step methods)

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngineModes" --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "bypass.*delta\|delta.*bypass\|bypass.*unity\|unity.*bypass\|delta.*unity\|unity.*delta\|all.*three\|simultaneous" M-LIM/tests/dsp/test_limiter_engine_modes.cpp -i` → Expected: >= 3

## Tests
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_bypass_takes_priority_over_delta` — bypass+delta → output ≈ input
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_bypass_takes_priority_over_unity` — bypass+unity → output ≈ input unchanged
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_delta_and_unity_compose_correctly` — delta+unity both active, output finite and bounded
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_all_three_flags_bypass_wins` — bypass+delta+unity → output ≈ input
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_mode_switch_delta_bypass_delta_no_large_jump` — no > 6 dB step across blocks

## Technical Details
- Use the same helper pattern as existing tests: `LimiterEngine engine; engine.prepare(44100.0, 2, 512);`
- For the priority test: feed a 0 dBFS sine, set bypass=true and delta=true, process 10 blocks. The output of each block should match the input to within floating-point rounding (< 1e-4 linear error per sample, accounting for lookahead delay).
- For delta+unity: feed a +6 dBFS signal (above ceiling). delta output = pre_limit - post_limited. Verify all output samples are finite and the RMS of delta output is greater than silence (confirming limiting is active).
- For the no-large-jump test: record max absolute sample across the boundary block between mode switches; require it to be < 2.0 (6 dBFS) rather than the full 0 dBFS → ΔdBFS jump that an unchecked transition could produce.

## Dependencies
None
