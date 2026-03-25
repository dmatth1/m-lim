# Task 180: Add Tests for TransientLimiter softSaturate() Path

## Description
Every test in `test_transient_limiter.cpp` sets `params.saturationAmount = 0.0f`, which causes
`softSaturate()` to exit on the early-return guard (`if (amount < 0.001f) return x`).  The actual
saturation formula — `wet = tanh(x * drive) / drive` blended by `amount` — has **zero test
coverage**.

Algorithms Punchy (0.3), Dynamic (0.2), Aggressive (0.8), Allround (0.4), Bus (0.7), and Modern
(0.1) all ship with non-zero `saturationAmount`, so this path executes in production for six of the
eight presets.

Tests to add (all in `test_transient_limiter.cpp`):

1. **test_saturation_reduces_peak** — Process a signal above threshold with
   `saturationAmount = 0.5f`.  Verify that the output peak is lower than it would be with
   `saturationAmount = 0.0f` (saturation compresses transients further).

2. **test_saturation_full_amount_no_crash** — Set `saturationAmount = 1.0f`, drive a loud signal
   (amplitude 2.0) through 10 blocks. Assert all output samples are finite (`std::isfinite`) and
   the peak does not exceed the threshold by more than a small tolerance.

3. **test_saturation_amount_zero_is_linear** — Explicitly confirm that `saturationAmount = 0.0f`
   produces the same output as not calling softSaturate at all; i.e., a sine wave below threshold
   is passed through unchanged (to serve as a regression baseline for the above tests).

4. **test_saturation_formula_direct** — Call `softSaturate` indirectly by setting
   `saturationAmount = 0.5f` and feeding a sub-threshold signal. Verify output magnitude is
   strictly less than input magnitude (tanh compression) when the input is large, and nearly equal
   to input when the input is small.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TransientLimiter.cpp:162-173` — softSaturate() implementation
Read: `M-LIM/src/dsp/LimiterAlgorithm.h:15-23` — AlgorithmParams struct, saturationAmount field
Modify: `M-LIM/tests/dsp/test_transient_limiter.cpp` — add the four tests above

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R test_transient_limiter --output-on-failure` → Expected: all tests pass including the four new ones
- [ ] Run: `grep -c "saturationAmount.*[^0]\." M-LIM/tests/dsp/test_transient_limiter.cpp` → Expected: output >= 3 (at least 3 tests use non-zero saturation)

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_saturation_reduces_peak` — non-zero saturation produces lower peak than zero saturation
- Unit: `tests/dsp/test_transient_limiter.cpp::test_saturation_full_amount_no_crash` — amount=1.0 stays finite and bounded
- Unit: `tests/dsp/test_transient_limiter.cpp::test_saturation_amount_zero_is_linear` — amount=0 is identity for sub-threshold signal
- Unit: `tests/dsp/test_transient_limiter.cpp::test_saturation_formula_direct` — compression visible on large sub-threshold signals

## Technical Details
- `softSaturate(x, amount)` at TransientLimiter.cpp:164: when `amount >= 0.001f`, computes
  `drive = 1 + amount*3`, `wet = tanh(x*drive)/drive`, returns `x*(1-amount) + wet*amount`.
- For large `|x|`, `tanh` saturates so output is less than input; for small `|x|` it approximates
  the identity.
- To exercise the path, set `params.saturationAmount` to a positive value **before** calling
  `limiter.setAlgorithmParams(params)`.  All existing tests override this to 0.0f.

## Dependencies
None
