# Task 011: Transient Limiter (Stage 1)

## Description
Implement the fast transient/peak limiter (Stage 1 of the dual-stage design). Uses a lookahead buffer to anticipate peaks and apply fast gain reduction.

## Produces
Implements: `TransientLimiterInterface`

## Consumes
AlgorithmDefinition

## Relevant Files
Create: `M-LIM/src/dsp/TransientLimiter.h` — class declaration
Create: `M-LIM/src/dsp/TransientLimiter.cpp` — implementation
Create: `M-LIM/tests/dsp/test_transient_limiter.cpp` — unit tests
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — AlgorithmParams struct
Read: `SPEC.md` — TransientLimiterInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_transient_limiter --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_peak_limiting` — input peak at +6dB should be limited to ~0dB output
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_anticipation` — with lookahead, gain reduction starts before the peak arrives
- Unit: `tests/dsp/test_transient_limiter.cpp::test_channel_linking` — at 100% link, both channels get same GR; at 0%, independent GR
- Unit: `tests/dsp/test_transient_limiter.cpp::test_no_clipping` — output should never exceed 1.0 (0dBFS)
- Unit: `tests/dsp/test_transient_limiter.cpp::test_passthrough_below_threshold` — signal below 0dBFS passes unchanged

## Technical Details
- Circular lookahead buffer sized to max lookahead time (5ms at max sample rate)
- Peak detection: scan lookahead buffer for maximum, compute required gain reduction
- Gain reduction smoothing: exponential attack (instant or very fast) and release (algorithm-dependent)
- Channel linking: weighted average of per-channel GR based on link percentage
- Gain is applied in linear domain (multiply, not add in dB)
- Soft knee: smooth transition from no-reduction to full-reduction based on AlgorithmParams.kneeWidth
- Saturation: optional soft clipping per AlgorithmParams.saturationAmount (tanh waveshaping)
- Must be real-time safe: no allocations in process()

## Dependencies
Requires tasks 001, 004
