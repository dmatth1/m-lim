# Task: DCFilter — Cutoff Frequency Characterization Tests

## Description
`DCFilter` is documented as a first-order high-pass with ~5 Hz cutoff
(`R = 1 - 2*pi*5/sampleRate`). The existing tests verify:
- DC removal (passes)
- 1 kHz passthrough (passes)
- Reset/reprepare/denormal (passes)

But no test verifies the actual cutoff behavior:
- At exactly 5 Hz, the filter should attenuate by ~3 dB (−3 dB is the definition of cutoff)
- At 1 Hz, attenuation should be much larger (>12 dB for a first-order HP)
- At 20 Hz, attenuation should be small (<1 dB) — audio content must pass

Without this, a regression that accidentally changes `kCutoffHz` from 5.0 to 50.0 (which
would noticeably affect low bass) would not be caught. A cutoff at 50 Hz would already
attenuate content audible to listeners.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/DCFilter.h` — transfer function: y[n] = x[n] - x[n-1] + R*y[n-1]
Read: `src/dsp/DCFilter.cpp` — kCutoffHz = 5.0, R computation
Read: `tests/dsp/test_dc_filter.cpp` — existing tests
Modify: `tests/dsp/test_dc_filter.cpp` — add cutoff characterization tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "DCFilter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_cutoff_near_5hz_minus3db` — feed 5 Hz sine for 44100 samples at 44100 Hz; measure RMS of last half; verify attenuation is in [-4.5, -1.5] dB (approximately -3 dB for first-order HP at cutoff)
- Unit: `tests/dsp/test_dc_filter.cpp::test_1hz_heavily_attenuated` — feed 1 Hz sine for 44100 samples; measure RMS of last half; verify attenuation > 12 dB (first-order HP at 1/5 of cutoff → 20*log10(5) ≈ 14 dB expected)
- Unit: `tests/dsp/test_dc_filter.cpp::test_20hz_passes_with_little_attenuation` — feed 20 Hz sine for 44100 samples; verify attenuation < 1.5 dB (20 Hz is 4× the cutoff; should pass cleanly)
- Unit: `tests/dsp/test_dc_filter.cpp::test_cutoff_scales_with_sample_rate` — prepare at 96000 Hz, feed 5 Hz sine; verify similar ~3 dB attenuation as at 44100 Hz (R recomputed correctly)

## Technical Details
Measuring RMS attenuation: use the latter half of the buffer (>100 cycles of 5 Hz
needed to reach steady state). Compute:
```cpp
double rmsIn = 1.0 / std::sqrt(2.0); // unit sine RMS
double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);
REQUIRE(attenuationDb > -4.5);
REQUIRE(attenuationDb < -1.5);
```
At 44100 Hz, 5 Hz needs 44100/5 = 8820 samples per cycle; use 88200+ samples
total to ensure steady state is reached. The tolerance ±1.5 dB accounts for
the first-order approximation `R ≈ 1 - 2πfc/fs` vs exact bilinear transform.

## Dependencies
None
