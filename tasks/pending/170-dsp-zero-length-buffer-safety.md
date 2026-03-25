# Task 170: Add Zero-Length Buffer Safety Tests for DSP Components

## Description
Several DSP components accept a `numSamples` parameter but no test ever passes `0`.  Code
inspection shows each component has an early-return guard or loop that would be skipped, but
**none of these guards are verified by tests**.  A future refactor that accidentally removes a
guard could introduce undefined behaviour without a failing test.

Components to cover and their guards:
- `DCFilter::process(float* data, int numSamples)` — DCFilter.cpp iterates `numSamples`
- `TruePeakDetector::processBlock(const float* input, int numSamples)` — TruePeakDetector.cpp
- `SidechainFilter::process(AudioBuffer<float>&)` — uses buffer.getNumSamples()

Tests to add:

**In `test_dc_filter.cpp`:**
1. **test_process_zero_samples_no_crash** — Call `process(buf, 0)` after `prepare()`. Assert no
   crash and the filter's internal state is unchanged (subsequent real processing still produces
   correct DC removal).

**In `test_true_peak.cpp`:**
2. **test_process_block_zero_samples_no_crash** — Call `processBlock(ptr, 0)` after `prepare()`.
   Assert `getPeak()` returns `0.0f` (unchanged from reset state) and no crash.

3. **test_process_block_single_sample** — Call `processBlock(ptr, 1)` with a known value. Assert
   `getPeak()` is >= 0.0 and finite; the interpolator must not read out of bounds.

**In `test_sidechain_filter.cpp`:**
4. **test_process_zero_sample_buffer_no_crash** — Prepare a `juce::AudioBuffer<float>` with 2
   channels and 0 samples; call `SidechainFilter::process()`. Assert no crash and the filter
   internal `dsp::IIR::Filter` state is not corrupted (process a real buffer next and verify
   finite output).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/DCFilter.cpp` — process loop, check for numSamples guard
Read: `M-LIM/src/dsp/TruePeakDetector.cpp:154-160` — processBlock entry, early-exit guard
Read: `M-LIM/src/dsp/SidechainFilter.cpp:60-80` — process(), how buffer size is used
Modify: `M-LIM/tests/dsp/test_dc_filter.cpp` — add test_process_zero_samples_no_crash
Modify: `M-LIM/tests/dsp/test_true_peak.cpp` — add two new tests
Modify: `M-LIM/tests/dsp/test_sidechain_filter.cpp` — add test_process_zero_sample_buffer_no_crash

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R "test_dc_filter|test_true_peak|test_sidechain_filter" --output-on-failure` → Expected: all tests pass including the four new tests
- [ ] Run: `grep -c "zero.*sample\|0.*sample\|numSamples.*0" M-LIM/tests/dsp/test_dc_filter.cpp M-LIM/tests/dsp/test_true_peak.cpp M-LIM/tests/dsp/test_sidechain_filter.cpp` → Expected: total >= 4

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_process_zero_samples_no_crash` — zero-length pass is safe and doesn't corrupt state
- Unit: `tests/dsp/test_true_peak.cpp::test_process_block_zero_samples_no_crash` — zero-length returns unchanged peak
- Unit: `tests/dsp/test_true_peak.cpp::test_process_block_single_sample` — single-sample doesn't OOB in interpolator
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_process_zero_sample_buffer_no_crash` — zero-sample buffer safe and state survives

## Technical Details
- For TruePeakDetector: the FIR convolution at processBlock iterates `numSamples`; if there's no
  guard, passing 0 is fine since the loop body never executes. Verify experimentally.
- For single-sample TruePeakDetector: the internal ring buffer must wrap correctly after 1 write;
  the result should be `0.0f` until the FIR delay is filled, but must not crash.
- For SidechainFilter: `juce::AudioBuffer<float>(2, 0)` creates a buffer with valid channel
  pointers but 0 samples; JUCE dsp::IIR::Filter may or may not handle this gracefully.

## Dependencies
None
