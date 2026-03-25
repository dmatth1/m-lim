# Task 197: Oversampler — amplitude preservation and latency reporting tests

## Description
`test_oversampler.cpp` only checks that upsampled block sizes are correct and that passthrough
is a no-op. It does not verify amplitude fidelity or latency reporting, which are critical for
a limiter (wrong latency → wrong lookahead compensation; amplitude error → false gain reduction).

Add tests for:

1. **Amplitude preservation round-trip** — for each oversampling factor (0–5), upsample a 440 Hz
   sine at 0.5 amplitude, then immediately downsample and verify the output RMS is within ±1 dB
   of the input RMS. This catches filter gain errors introduced by the oversampling chain.

2. **Latency is non-negative and increases with factor** — call `getLatencySamples()` for factors
   0–5 after `prepare()`. Verify: factor 0 returns 0.0, and each higher factor returns ≥ the
   previous factor's latency. Latency must also be a finite, positive (or zero) value.

3. **Factor 0 passthrough is truly lossless** — upsample + downsample with factor=0 must produce
   output sample-for-sample identical to the input (max absolute diff < 1e-6f). This is stronger
   than the existing "dimensions unchanged" check.

4. **setFactor() outside [0, 5] is safe** — call `setFactor(-1)`, `setFactor(6)`, and
   `setFactor(100)` after prepare(); verify the component does not crash and `getFactor()` either
   clamps to the valid range or returns the last valid factor.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/dsp/test_oversampler.cpp` — add new TEST_CASEs
Read: `src/dsp/Oversampler.h` — API: prepare(), upsample(), downsample(), setFactor(), getFactor(),
    getLatencySamples(), kMinOversamplingFactor, kMaxOversamplingFactor

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "Oversampler" --output-on-failure` → Expected: all oversampler tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_oversampler.cpp::test_amplitude_preservation_all_factors` — round-trip RMS within ±1 dB for factors 1–5
- Unit: `tests/dsp/test_oversampler.cpp::test_latency_monotonically_increases` — latency(0)=0, latency(N) >= latency(N-1) for N=1..5
- Unit: `tests/dsp/test_oversampler.cpp::test_passthrough_lossless` — factor=0 output is bit-identical to input (diff < 1e-6)
- Unit: `tests/dsp/test_oversampler.cpp::test_set_factor_out_of_range_safe` — negative and >5 factors do not crash; getFactor() stays in [0,5]

## Technical Details
- For amplitude test: use `kBlockSize = 512` samples of a 440 Hz sine. Compute RMS before upsample
  and after downsample; convert to dB: `20*log10(rms_out/rms_in)` must be in [-1.0, +1.0].
- For the latency test, use a loop: `for (int f = 0; f <= 5; ++f) { os.setFactor(f); ... }`.
  Note that `setFactor()` re-allocates — only call it outside the audio thread (in test setup).
- Re-call `os.prepare()` after each `setFactor()` if needed to reset internal state.
- The existing helpers in test_oversampler.cpp are minimal; add your own `computeRMS()` helper inline.

## Dependencies
None
