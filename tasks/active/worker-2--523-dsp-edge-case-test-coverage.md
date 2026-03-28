# Task 523: DSP Edge Case Test Coverage

## Description
Consolidate four edge-case test gaps across DSP modules: dither intermediate bit depths (18/20/22-bit), limiter multichannel (3+ channels), oversampler out-of-range factor clamping, and sidechain filter extreme sample rates. These are all small test additions that cover boundary conditions not yet exercised.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/tests/dsp/test_dither.cpp` — add 18/20/22-bit quantization step tests
Modify: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — add 4-channel and 6-channel tests
Modify: `M-LIM/tests/dsp/test_transient_limiter.cpp` — add 4-channel and 6-channel tests
Modify: `M-LIM/tests/dsp/test_oversampler.cpp` — add factor clamping and zero-sample buffer tests
Modify: `M-LIM/tests/dsp/test_sidechain_filter.cpp` — add 8kHz and 384kHz sample rate stability tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_18bit_quantization_step` — verify 18-bit quantization step
- Unit: `tests/dsp/test_dither.cpp::test_20bit_quantization_step` — verify 20-bit quantization step
- Unit: `tests/dsp/test_dither.cpp::test_22bit_quantization_step` — verify 22-bit quantization step
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_4_channel_no_crash` — 4 channels, verify output finite
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_6_channel_linking_works` — 6 channels with linking
- Unit: `tests/dsp/test_transient_limiter.cpp::test_4_channel_peak_limiting` — 4 channels
- Unit: `tests/dsp/test_oversampler.cpp::test_negative_factor_no_crash` — setFactor(-1) clamped
- Unit: `tests/dsp/test_oversampler.cpp::test_factor_above_max_no_crash` — setFactor(6+) clamped
- Unit: `tests/dsp/test_oversampler.cpp::test_zero_sample_buffer_upsample` — 0-sample buffer no crash
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_8khz_sample_rate_stable` — HP near Nyquist at 8kHz
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_384khz_sample_rate_stable` — stability at 384kHz

## Technical Details
For dither: process known signal with each bit depth, verify quantization step matches `1 / (2^(N-1) - 1)`.
For multichannel: prepare with 4 and 6 channels, process loud signal, verify all outputs finite and limited.
For oversampler: verify factor is clamped to [0, 5] range. Verify 0-sample and 1-sample buffers don't crash.
For sidechain: at 8kHz, HP=2000Hz is near Nyquist — verify no NaN. At 384kHz, verify stable output.

## Dependencies
None
