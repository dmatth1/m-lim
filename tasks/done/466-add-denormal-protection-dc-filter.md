# Task 466: Add Explicit Denormal Protection to DCFilter IIR State

## Description
The `DCFilter::process()` in `DCFilter.cpp` (line 14-25) relies on `juce::ScopedNoDenormals` to set FTZ/DAZ CPU flags. While this prevents denormal slowdowns during processing, the filter state variables `yPrev` (float) can accumulate denormal values during silence (zero input) between blocks — after `ScopedNoDenormals` goes out of scope and before the next `process()` call. On some platforms or in some host configurations, the FTZ/DAZ flags might be cleared between audio callbacks.

**The issue**: The DC filter's feedback coefficient `R` is very close to 1.0 (0.9993 at 44.1kHz). During silence, `yPrev` decays as `R^n * yPrev`, which will eventually reach denormal territory (~1e-38). On the next process() call, if FTZ/DAZ flags aren't set yet (before ScopedNoDenormals constructor runs), the first multiply `R * yPrev` with a denormal `yPrev` could cause a stall.

**Fix**: Add a tiny DC bias in the feedback path, similar to what the LoudnessMeter's Biquad does with `kDenormalFix = 1e-25`:

```cpp
float y = x - xPrev + static_cast<float>(R) * yPrev;
// Add anti-denormal bias (alternating sign to cancel out):
yPrev = y + 1e-25f;  // or use alternating sign
```

Or more robustly, snap `yPrev` to zero when it drops below a threshold:
```cpp
yPrev = y;
if (std::abs(yPrev) < 1e-15f) yPrev = 0.0f;
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/DCFilter.cpp` — add denormal protection in process() loop (line 20-21)

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_denormal_after_silence` — process 10000 samples of silence, verify yPrev does not contain denormal values

## Technical Details
- This is a one-line addition to the inner loop
- The alternating-sign bias approach (add +1e-25 one sample, -1e-25 the next) is cleanest as it has zero DC offset
- Priority is low since ScopedNoDenormals covers the common case, but this provides defense-in-depth

## Dependencies
None
