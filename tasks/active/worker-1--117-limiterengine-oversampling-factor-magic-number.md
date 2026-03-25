# Task 117: LimiterEngine::setOversamplingFactor() Uses Magic Numbers for Valid Range

## Description
`LimiterEngine::setOversamplingFactor()` (`LimiterEngine.cpp` line 396) clamps
the input with hard-coded integers:

```cpp
mOversamplingFactor.store(std::max(0, std::min(5, factor)));
```

The magic numbers `0` and `5` represent the minimum and maximum valid
oversampling-factor indices but are not explained or centrally defined.  If the
`Oversampler` class is extended to support more factors, only this line (and any
other guesses) would need updating — with no compile-time guidance pointing there.

The same constant `5` implicitly appears in `PluginProcessor.cpp` and
`Parameters.cpp` where the oversampling combo box is populated.

Fix:
1. Add to `Oversampler.h` (or `LimiterAlgorithm.h`) a pair of constants:
   ```cpp
   static constexpr int kMinOversamplingFactor = 0;
   static constexpr int kMaxOversamplingFactor = 5;
   ```
2. Use these constants in `LimiterEngine::setOversamplingFactor()`.
3. Optionally add a `static_assert` in `Oversampler.cpp` that validates the
   constants match the actual switch/case range.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/Oversampler.h` — add kMinOversamplingFactor / kMaxOversamplingFactor constants
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — replace magic 0 and 5
Read: `M-LIM/src/Parameters.cpp` — check if the same range is duplicated in parameter layout

## Acceptance Criteria
- [ ] Run: `grep -n "std::min(5\|std::max(0" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no matches
- [ ] Run: `grep -n "kMaxOversamplingFactor\|kMinOversamplingFactor" M-LIM/src/dsp/Oversampler.h` → Expected: both constants present
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds

## Tests
None — pure naming refactor; no logic changes.

## Technical Details
- Constants should be `static constexpr int` and placed in `Oversampler.h`
  because they describe the valid range of the Oversampler API.
- Factor 0 = 1x (no oversampling), factor 5 = 32x.  Consider adding a comment
  documenting this.

## Dependencies
None
