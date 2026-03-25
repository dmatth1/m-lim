# Task 127: LimiterEngine::process() Contains Three Identical Peak-Scan Loops — Extract Helper

## Description
`LimiterEngine::process()` (`LimiterEngine.cpp`) contains three nearly-identical
per-channel peak-scan loops:

- Lines 162–170: scan input buffer for pre-gain peak levels
- Lines 178–185: scan bypass output buffer for peak levels
- Lines 300–306: scan post-processing output buffer for peak levels

Each loop follows the same pattern:
```cpp
float level = 0.0f;
for (int i = 0; i < numSamples; ++i)
    level = std::max(level, std::abs(buffer.getSample(ch, i)));
```

This violates DRY and makes the 199-line `process()` function harder to read.

Fix: Add a private static helper to `LimiterEngine`:
```cpp
static float peakLevel(const juce::AudioBuffer<float>& buf,
                        int channel, int numSamples) noexcept;
```
Replace all three loops with calls to this helper.  Do not change any logic,
variable names at call sites, or the surrounding meter-data assembly code.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.h` — add private static declaration
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — add implementation, replace three loop bodies

## Acceptance Criteria
- [ ] Run: `grep -c "for (int i = 0; i < numSamples; ++i)" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: 0 (all inline scan loops replaced)
- [ ] Run: `grep -c "peakLevel" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: at least 4 (1 definition + 3 call sites)
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds
- [ ] Run: `cd M-LIM && cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None — pure refactor; existing tests validate correctness.

## Technical Details
- The helper should be `static` (no instance state) and `noexcept`.
- Signature should accept `const juce::AudioBuffer<float>&`, not a raw pointer, to match existing call sites.
- Place the implementation before `process()` in LimiterEngine.cpp.

## Dependencies
None
