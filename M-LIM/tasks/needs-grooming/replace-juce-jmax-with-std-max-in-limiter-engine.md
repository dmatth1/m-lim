# Task: Replace `juce::jmax` with `std::max` in LimiterEngine.cpp DSP Code

## Description
`LimiterEngine.cpp` line 237 uses `juce::jmax` in the middle of the DSP processing chain:

```cpp
const float totalGR = juce::jmax(
    mTransientLimiter.getGainReduction() + mLevelingLimiter.getGainReduction(), -60.0f);
```

Every other min/max operation in the DSP layer (`LimiterEngine.cpp`, `TransientLimiter.cpp`,
`LevelingLimiter.cpp`, `DspUtil.h`) uses `std::max` / `std::min` / `std::clamp`. The DSP files
deliberately minimise JUCE header dependencies (they use `juce_audio_basics` only for buffer
types, not for math helpers). Using `juce::jmax` here is inconsistent and introduces an
unnecessary JUCE dependency in a pure-math expression.

`juce::jmax(a, b)` is equivalent to `std::max(a, b)` for arithmetic types; the replacement
is trivially safe.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — change `juce::jmax(...)` to `std::max(...)` at line 237

## Acceptance Criteria
- [ ] Run: `grep "juce::jmax\|juce::jmin" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (no behavior change — `juce::jmax` and `std::max` are equivalent for float)

## Technical Details
- Change only the one call site in `LimiterEngine.cpp` line 237; do not modify UI files
  (`RotaryKnob.cpp`, `GainReductionMeter.cpp`, `PluginEditor.cpp`) which legitimately use JUCE
  math helpers since they already depend heavily on JUCE
- `std::max` is available via `<algorithm>` which `LimiterEngine.cpp` already includes (line 6)

## Dependencies
None
