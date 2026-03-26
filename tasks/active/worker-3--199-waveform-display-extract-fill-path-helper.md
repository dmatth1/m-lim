# Task 199: Extract Repeated Fill-Path Loop in WaveformDisplay

## Description
`WaveformDisplay::paint()` dispatches to four private draw methods — `drawOutputFill()`, `drawInputFill()`, `drawGainReduction()`, and `drawOutputEnvelope()` — all of which contain nearly identical path-building loops over the waveform frame buffer. The structure is:

```cpp
juce::Path p;
p.startNewSubPath(...);
for (int x = ...; x < ...; ++x) {
    // compute y from frame[x]
    p.lineTo(float(x), y);
}
p.lineTo(...); // close bottom
p.closeSubPath();
g.setColour(...);
g.fillPath(p);
```

This pattern is repeated four times with minor variations (different data accessor, different colour). Extract a private helper that accepts a lambda for the per-column value and the fill colour, and call it from the four draw methods.

Suggested signature:
```cpp
void drawFilledWaveformPath(juce::Graphics& g,
                            juce::Colour colour,
                            std::function<float(int x)> columnValue,
                            juce::Rectangle<int> bounds);
```

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/WaveformDisplay.h` — find existing private method declarations
Modify: `M-LIM/src/ui/WaveformDisplay.h` — add `drawFilledWaveformPath()` private declaration
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — implement helper; refactor four draw methods to use it

## Acceptance Criteria
- [ ] Run: `grep -c "startNewSubPath\|lineTo\|closeSubPath" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: count is lower than before refactor (duplicated path-building removed)
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `source M-LIM/Scripts/ui-test-helper.sh && start_app && screenshot screenshots/199-after.png && stop_app` → Expected: waveform renders identically to before

## Tests
None (visual regression covered by screenshot comparison)

## Technical Details
- The lambda captures nothing from the caller; it just maps an x column index to a normalised y value (0.0–1.0 or dB).
- If `std::function` overhead is a concern, use a template parameter instead: `template<typename Fn> void drawFilledWaveformPath(...)`. Either is acceptable.
- Colours passed as parameters; do not read from members inside the helper.

## Dependencies
None
