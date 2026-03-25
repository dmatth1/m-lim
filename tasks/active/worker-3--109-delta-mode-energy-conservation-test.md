# Task 109: LimiterEngine Delta Mode Energy Conservation Test

## Description
The existing `test_delta_outputs_difference` and `test_delta_silence_when_no_limiting`
tests only verify that the delta output is non-zero (or near-zero). They do **not** verify
the fundamental correctness property of delta mode:

```
delta_output + limited_output ≈ input_after_gain
```

Without this invariant test, a broken delta implementation that outputs random noise would
still pass all current assertions. Add a test that:

1. Captures the limited output (with delta mode OFF)
2. Captures the delta output (with delta mode ON, same input)
3. Verifies that for each sample: `delta[i] + limited[i] ≈ input_gained[i]`

Because the limiter is non-causal (lookahead introduces delay), both runs must use the same
number of warm-up blocks so that the internal states are comparable. The most practical
approach is to run the engine twice with the same seed, one in each mode, and compare sample
by sample after warm-up has settled.

**Addditionally** add a test that verifies delta mode output is always bounded (no runaway
values when heavily limited, since the subtraction could theoretically overflow float range).

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.h` — setDeltaMode, process
Read: `src/dsp/LimiterEngine.cpp` — delta mode implementation (pre-limit snapshot and subtraction)
Modify: `tests/dsp/test_limiter_engine_modes.cpp` — add energy-conservation and bounded-output tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LimiterEngineModes" --output-on-failure` → Expected: all tests pass, including new tests
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_delta_plus_limited_equals_input` — `delta[i] + limited[i]` equals pre-limit input within 1e-4 tolerance for each sample
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_delta_output_bounded` — with 100x amplitude input and delta mode enabled, output stays within ±10.0f (no NaN/Inf, no explosions)

## Technical Details
The delta mode in LimiterEngine.cpp (steps ~197–201):
```cpp
if (mDeltaMode.load())
{
    for (int ch = 0; ch < numChannels; ++ch)
        mPreLimitBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
}
```
After processing, it subtracts limited from pre-limit. The conservation test needs two identical
engines processed with the same input — one in normal mode, one in delta mode — and verifies
that `normal[i] + delta[i] ≈ preLimit[i]`.

Since the lookahead delay offsets timing, the simplest approach is to run a single engine in
delta mode and verify that `|delta_output[i]| <= |input_gained[i]| + epsilon` (delta can't
remove more than what was there).

## Dependencies
None
