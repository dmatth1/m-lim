# Task 223: Fix juce::FontOptions JUCE 7 API Incompatibility

## Description
The codebase uses `juce::FontOptions` which was introduced in JUCE 8 and does not exist in JUCE 7.0.12. This causes a compile error on all platforms. The fix was partially applied by the orchestrator (ControlStrip.cpp), but this task ensures a full audit of the entire codebase and confirms the build compiles cleanly.

All occurrences of:
```cpp
juce::Font(juce::FontOptions().withHeight(h).withStyle("Bold"))
```
must be replaced with the JUCE 7 equivalent:
```cpp
juce::Font(h, juce::Font::bold)
```
Or for plain (non-bold):
```cpp
juce::Font(h)
```

The orchestrator already fixed 2 occurrences in `src/ui/ControlStrip.cpp`. This task audits for any remaining occurrences and verifies the build compiles.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — already fixed, verify
Read: `src/ui/LookAndFeel.cpp` — check for any FontOptions usage
Read: `src/ui/RotaryKnob.cpp` — check for any FontOptions usage
Read: `src/ui/WaveformDisplay.cpp` — check for any FontOptions usage
Read: `src/ui/LevelMeter.cpp` — check for any FontOptions usage
Read: `src/ui/GainReductionMeter.cpp` — check for any FontOptions usage
Read: `src/ui/TopBar.cpp` — check for any FontOptions usage
Read: `src/ui/LoudnessPanel.cpp` — check for any FontOptions usage
Read: `src/PluginEditor.cpp` — check for any FontOptions usage

## Acceptance Criteria
- [ ] Run: `grep -r "FontOptions" /workspace/M-LIM/src/ --include="*.h" --include="*.cpp"` → Expected: no output (zero matches)
- [ ] Run: `export CCACHE_DIR=/build-cache && cmake -B /workspace/M-LIM/build -S /workspace/M-LIM -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && cmake --build /workspace/M-LIM/build -j$(nproc) 2>&1 | tail -20` → Expected: build succeeds, exit 0, no FontOptions errors

## Tests
None

## Technical Details
In JUCE 7, Font construction:
- `juce::Font(float height)` — plain weight
- `juce::Font(float height, int styleFlags)` — with style; `juce::Font::bold`, `juce::Font::italic`, `juce::Font::underlined`
- `font.withHeight(h)` — fluent copy-style

## Dependencies
None
