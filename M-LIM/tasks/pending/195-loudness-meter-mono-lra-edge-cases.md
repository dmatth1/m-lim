# Task 195: LoudnessMeter — mono channel, LRA edge-case, and multichannel tests

## Description
`test_loudness_meter.cpp` and `test_loudness_meter_accuracy.cpp` only exercise stereo (2-channel)
input. The `LoudnessMeter` header explicitly supports arbitrary channel counts. Add tests for:

1. **Mono input** — `prepare(fs, 1)` then feed a 1 kHz sine and verify LUFS reads within ±1.5 LU
   of expected (stereo sums left+right; mono uses only channel 0, so expected LUFS differs by 3 dB).
2. **`resetIntegrated()` idempotency** — call `resetIntegrated()` on a fresh meter (never fed data)
   and then again after feeding data and resetting; both must not crash and must return -inf for
   integrated LUFS afterwards.
3. **LRA boundary conditions** — `getLoudnessRange()` on a meter fed uniform-level content
   (constant amplitude) must return 0.0 LU (or very near 0), because there is no variation.
4. **`prepare()` re-call changes channel count** — call `prepare(fs, 2)`, feed stereo data, then
   call `prepare(fs, 1)`, feed mono data; LUFS must reflect new channel count and not crash.
5. **Loudness meter tolerance tightening** — the existing `test_1khz_sine_loudness` allows ±1.5 LU.
   This is quite wide. Add a parallel test using the `expectedLufs()` helper from
   `test_loudness_meter_accuracy.cpp` with ±0.5 LU tolerance at steady-state integrated LUFS.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LoudnessMeter.h` — public API: prepare(), processBlock(), getMomentaryLUFS(),
    getShortTermLUFS(), getIntegratedLUFS(), getLoudnessRange(), resetIntegrated()
Modify: `tests/dsp/test_loudness_meter.cpp` — add new TEST_CASEs here
Read: `tests/dsp/test_loudness_meter_accuracy.cpp` — borrow the `expectedLufs()` helper and
    feedSine()/feedSilence() patterns

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LoudnessMeter" --output-on-failure` → Expected: all loudness-meter tests pass, exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LoudnessMeterAccuracy" --output-on-failure` → Expected: all accuracy tests still pass

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_mono_loudness` — mono -20 dBFS 1 kHz sine reads within ±1.5 LU of expected mono LUFS
- Unit: `tests/dsp/test_loudness_meter.cpp::test_reset_integrated_idempotent` — double resetIntegrated() does not crash; getIntegratedLUFS() returns -inf after each call
- Unit: `tests/dsp/test_loudness_meter.cpp::test_lra_constant_signal` — uniform level yields getLoudnessRange() ≈ 0.0 LU (< 0.5 LU)
- Unit: `tests/dsp/test_loudness_meter.cpp::test_reprepare_channel_count_change` — stereo → mono re-prepare works without crash or stale data
- Unit: `tests/dsp/test_loudness_meter.cpp::test_integrated_lufs_tight_tolerance` — integrated LUFS for -23 LUFS reference tone lands within ±0.5 LU

## Technical Details
- Mono LUFS formula: `LUFS = -0.691 + 10*log10(power_ch0)` — only one channel contributes, so
  level is -3 dB relative to correlated stereo at same amplitude.
- For the LRA constant-signal test: feed 20 s of -20 dBFS 1 kHz stereo sine (enough to populate
  history), then call `getLoudnessRange()`.
- `feedSilence` helper in the file already supports `numChannels` parameter — use `numChannels=1`
  for mono tests.
- Use `CHECK` not `REQUIRE` where failure of one assertion should not abort the test case.

## Dependencies
None
