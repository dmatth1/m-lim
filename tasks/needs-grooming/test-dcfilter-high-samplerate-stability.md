# Task: DCFilter — High Sample Rate R Coefficient Stability Tests

## Description
DCFilter computes `R = 1 - (2π * 5Hz / sampleRate)`. At very high sample rates (192 kHz), R approaches 0.9998, which is fine. At very low sample rates (e.g., 8000 Hz), R = 1 - 0.00393 ≈ 0.996, still stable. However, there are no tests that verify:
1. The `R` coefficient clamps correctly (no test at sample rates < 8 kHz or > 192 kHz).
2. Multiple `prepare()` calls (without reset) correctly reinitialize `R` and flush state.
3. The filter is stable at 192 kHz (long settling time — ensure DC is actually removed within a reasonable window).
4. `process()` with a nullptr data pointer or negative `numSamples` doesn't corrupt state (defensive boundary test).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/DCFilter.h` — `R` coefficient, `xPrev`, `yPrev`
Read: `M-LIM/src/dsp/DCFilter.cpp` — `prepare()` implementation
Modify: `M-LIM/tests/dsp/test_dc_filter.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "DCFilter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_high_samplerate_dc_removed_192khz` — At 192 kHz, feed 192000 samples (1 second) of DC offset 0.5f; measure DC in the latter half; require < 1e-3.
- Unit: `tests/dsp/test_dc_filter.cpp::test_reprepare_clears_state` — Call `prepare(44100)`, process 1000 samples of DC=1.0. Call `prepare(48000)` (no explicit reset). Process 100 samples of zeros. Verify output[99] is within 1e-5 of 0.0 (state was cleared by prepare).
- Unit: `tests/dsp/test_dc_filter.cpp::test_zero_numsamples_noop` — Already exists; verify state truly unchanged by checking yPrev indirectly via a subsequent known-signal process.
- Unit: `tests/dsp/test_dc_filter.cpp::test_single_sample_blocks` — Process 44100 single-sample blocks of DC=0.5; after the last block, accumulated DC in output should be < 1e-3 (same as the bulk test — ensures no off-by-one in loop).

## Technical Details
- DCFilter::prepare() already calls reset(), so re-prepare should clear state. The test confirms this is actually true.
- 192 kHz stability test: the time constant τ = 1/(2π*5) ≈ 31.8 ms. At 192 kHz, settling still occurs within 200 ms so 1 second is sufficient.

## Dependencies
None
