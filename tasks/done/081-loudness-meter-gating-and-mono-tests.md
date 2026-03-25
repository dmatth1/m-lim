# Task 081: LoudnessMeter Gating Accuracy, Mono Input, and Window Boundary Tests

## Description
`test_loudness_meter.cpp` and `test_loudness_meter_accuracy.cpp` test loudness in normal operating conditions but miss several important edge cases for ITU-R BS.1770-4 compliance:

- Mono input (1-channel buffer): meter must not crash and must return valid LUFS readings
- Absolute gate threshold: blocks below -70 LUFS must be excluded from integrated measurement
- Momentary window exact boundary: a signal lasting exactly 400 ms should produce a valid momentary reading
- Reset integrated only: other window types (momentary, short-term) should not be affected by `resetIntegrated()`
- Silence for entire session: `getIntegratedLoudness()` should return -infinity (not a large negative number with noise)
- LRA value is 0 when signal is constant (no variation)

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LoudnessMeter.h` — prepare(), processSamples(), getMomentaryLoudness(), getShortTermLoudness(), getIntegratedLoudness(), getLoudnessRange(), reset() interface
Read: `src/dsp/LoudnessMeter.cpp` — gating and window implementation
Modify: `tests/dsp/test_loudness_meter.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LoudnessMeter --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/dsp/test_loudness_meter.cpp` → Expected: at least 10 test cases

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_mono_input_no_crash` — prepare() with 1 channel, process 100 blocks of sine, getIntegratedLoudness() is finite and negative
- Unit: `tests/dsp/test_loudness_meter.cpp::test_absolute_gate_excludes_silence` — feed 5 seconds of silence then 3 seconds of -20 LUFS signal; integrated result should reflect only the -20 LUFS period (not dragged down to -infinity by the silent period)
- Unit: `tests/dsp/test_loudness_meter.cpp::test_reset_integrated_does_not_affect_momentary` — process a signal, call resetIntegrated(), continue processing; getMomentaryLoudness() is still valid
- Unit: `tests/dsp/test_loudness_meter.cpp::test_lra_zero_for_constant_signal` — 10 seconds of constant -23 LUFS signal → getLoudnessRange() is approximately 0 (< 0.5 LU)
- Unit: `tests/dsp/test_loudness_meter.cpp::test_silence_integrated_is_minus_infinity` — process only silence for entire session; getIntegratedLoudness() returns -infinity (std::isinf check)

## Technical Details
- For mono test: `juce::AudioBuffer<float> buf(1, blockSize)` — verify prepare() accepts this
- For absolute gate test: feed 5*44100/512 ≈ 430 blocks of silence followed by 260 blocks of a calibrated -20 LUFS sine. Integrated result should be within ±2 LU of -20 LUFS.
- For silence test: `std::isinf(getIntegratedLoudness())` should be true (negative infinity)

## Dependencies
None
