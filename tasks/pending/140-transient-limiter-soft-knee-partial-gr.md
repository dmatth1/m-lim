# Task 140: TransientLimiter — Missing Tests for Soft Knee Partial Gain Reduction

## Description
`TransientLimiter::computeRequiredGain()` implements a soft-knee curve: signals between
`threshold - kneeWidth/2` and `threshold + kneeWidth/2` receive partial gain reduction
(transitional zone), while signals below the knee should receive zero reduction and signals
well above should receive full reduction.

No test currently validates this behaviour:
- `test_peak_limiting` only verifies that a +6 dBFS input is clamped — it doesn't verify the
  knee transition zone.
- `test_custom_threshold_minus_1dBFS` / `test_custom_threshold_minus_6dBFS` only test the
  LevelingLimiter (Stage 2), not the TransientLimiter (Stage 1).

Bugs this would catch:
1. The knee curve formula could be accidentally inverting direction (applying full GR below
   the knee, zero above — sign error).
2. A regression where `kneeWidth` defaults to 0, making the knee a hard cliff with no
   partial reduction.
3. Signals just below the knee being incorrectly attenuated (false limiting).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TransientLimiter.h` — `computeRequiredGain`, `AlgorithmParams.kneeWidth`
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — `AlgorithmParams` struct and default kneeWidth
Read: `M-LIM/src/dsp/TransientLimiter.cpp` — implementation of computeRequiredGain
Modify: `M-LIM/tests/dsp/test_transient_limiter.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R TransientLimiter --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_below_knee_no_gr` — set threshold to 1.0,
  kneeWidth to the Transparent algorithm default; feed a signal at amplitude 0.5 (well below
  knee); verify `getGainReduction()` is approximately 0 dB (< 0.1 dB reduction) after warm-up.
- Unit: `tests/dsp/test_transient_limiter.cpp::test_in_knee_partial_gr` — feed a signal at the
  midpoint of the knee (threshold - kneeWidth/4); verify GR is between -0.1 dB and the full
  required dB reduction (partial, not zero, not full).
- Unit: `tests/dsp/test_transient_limiter.cpp::test_above_knee_full_gr` — feed a signal well
  above the knee (threshold × 2); verify that after warm-up, output is clamped to ≤ threshold
  + small margin, confirming full GR is applied.

## Technical Details
- Use `getAlgorithmParams(LimiterAlgorithm::Transparent)` to get a params struct with a
  non-zero `kneeWidth` value, then pass it to `setAlgorithmParams()`.
- The knee region in dB is [threshold_dB - kneeWidth/2, threshold_dB + kneeWidth/2].
- For the in-knee test: choose an input amplitude such that its dBFS value lies exactly at
  `threshold_dB - kneeWidth/4` (one-quarter of the way through the knee from below).
- Warm up the limiter for several blocks with the test signal before measuring GR, to ensure
  the smoothed gain state has reached steady state.

## Dependencies
None
