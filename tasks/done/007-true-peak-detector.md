# Task 007: True Peak Detector

## Description
Implement ITU-R BS.1770-4 compliant true peak detection using 4x oversampled FIR interpolation to detect inter-sample peaks.

## Produces
Implements: `TruePeakDetectorInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/TruePeakDetector.h` — class declaration
Create: `M-LIM/src/dsp/TruePeakDetector.cpp` — implementation
Create: `M-LIM/tests/dsp/test_true_peak.cpp` — unit tests
Read: `SPEC.md` — TruePeakDetectorInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_true_peak --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_true_peak.cpp::test_detects_intersample_peak` — two successive samples at 0.7 that would produce ~1.0 inter-sample peak; verify detection > 0.9
- Unit: `tests/dsp/test_true_peak.cpp::test_sample_peak_matches` — single sample at 0.5; true peak should be >= 0.5
- Unit: `tests/dsp/test_true_peak.cpp::test_reset_clears_peak` — verify reset() sets peak to 0
- Unit: `tests/dsp/test_true_peak.cpp::test_block_processing` — process a block and verify getPeak() returns max

## Technical Details
- 4x oversampling with 48-tap FIR interpolation filter (ITU-R BS.1770-4 coefficients)
- The FIR filter is a 4-phase polyphase decomposition of a 48-tap low-pass
- For each input sample, compute 4 interpolated values and take the max absolute value
- Store running peak value, reset on demand
- processBlock processes an entire buffer and updates internal peak
- processSample returns the true peak for a single sample

## Dependencies
Requires task 001
