# Task 201: DCFilter and Dither — edge-case and boundary tests

## Description
`test_dc_filter.cpp` and `test_dither.cpp` have no negative tests or boundary checks. The
existing `test_dsp_edge_cases.cpp` covers zero-length and single-sample buffers but not invalid
configuration or numerical boundary scenarios.

Add tests for:

### DCFilter
1. **Repeated `prepare()` without `reset()`** — call `prepare()` twice in a row with different
   sample rates (44100 then 96000); process a signal after the second prepare; output must be
   finite and the DC offset must be filtered (a constant 0.5f signal must trend toward 0.0f).
2. **DC removal correctness at extreme sample rates** — test at 8000 Hz and 192000 Hz; feed
   100 blocks of a 0.3f constant signal and verify that the final block's mean approaches 0.0f
   (DC is attenuated by > 20 dB).
3. **Large impulse does not produce INF/NAN** — feed a single sample of 1e10f (extreme amplitude);
   output must be finite.

### Dither
1. **TPDF noise density** — with bit depth 16, the output variance (measured over 10000 samples
   of silence input) should be in the range corresponding to ±1 LSB of 16-bit (approximately
   [1e-10, 1e-7]). This validates that noise is being added at the correct magnitude.
2. **`process()` with zero-amplitude input** — feed 1000 samples of exactly 0.0f; all output
   samples must be finite and in the range [-2 LSB, +2 LSB] for the configured bit depth.
3. **Repeated `prepare()` stability** — call `prepare(44100)`, then `prepare(96000)`, then
   `prepare(44100)` again; process 512 samples of silence; output must be finite (no state corruption).
4. **Noise shaping produces correct spectral tilt** — with noise shaping enabled (if the API
   supports it), high-frequency energy should exceed low-frequency energy: compute RMS of the
   first quarter of the frequency spectrum and last quarter, verify the last quarter is higher.
   If noise shaping is not separately configurable, skip this test with `SKIP("no noise shaping API")`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/dsp/test_dc_filter.cpp` — add new TEST_CASEs
Modify: `tests/dsp/test_dither.cpp` — add new TEST_CASEs
Read: `src/dsp/DCFilter.h` — prepare(), reset(), process()
Read: `src/dsp/Dither.h` — prepare(), process(), and any noise-shaping / bit-depth APIs

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "DCFilter" --output-on-failure` → Expected: all DCFilter tests pass, exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "Dither" --output-on-failure` → Expected: all Dither tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_reprepare_different_sample_rate` — two prepare() calls, DC filtered correctly after second prepare
- Unit: `tests/dsp/test_dc_filter.cpp::test_dc_attenuation_at_8khz` — 8000 Hz sample rate, constant 0.3f atenuated > 20 dB
- Unit: `tests/dsp/test_dc_filter.cpp::test_dc_attenuation_at_192khz` — 192000 Hz sample rate, same check
- Unit: `tests/dsp/test_dc_filter.cpp::test_large_impulse_finite` — 1e10f impulse produces finite output
- Unit: `tests/dsp/test_dither.cpp::test_tpdf_noise_density` — silence input with 16-bit depth, output variance in expected LSB range
- Unit: `tests/dsp/test_dither.cpp::test_zero_amplitude_in_range` — zero input, all output samples finite and within ±2 LSB
- Unit: `tests/dsp/test_dither.cpp::test_reprepare_stability` — three prepare() calls, output finite
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_spectral_tilt` — noise shaping produces spectral tilt toward HF (or skip if API absent)

## Technical Details
- For DC attenuation test: compute `mean_out = sum(samples) / N` over the last block (after
  the filter has had time to converge). Verify `|mean_out| < 0.3f * pow(10, -20.0/20.0)` (−20 dB).
- For the TPDF density test: 1 LSB for 16-bit at full-scale ≈ `1.0f / 32768.0f ≈ 3e-5f`. Variance
  of uniform distribution in [-1LSB, +1LSB] = `(2LSB)^2 / 12`. Expected variance ≈ `3e-10`.
  Accept range [1e-11, 1e-8] to account for TPDF (sum of two uniform) vs simple uniform.
- Check `Dither.h` for the actual API before writing tests — look for `setBitDepth()`, `setMode()`,
  `setNoiseShaping()` or similar. Only test what the API exposes.
- Use `SKIP(reason)` (Catch2 v3 syntax) for any test that is inapplicable given the current API.

## Dependencies
None
