# Task 221: Replace Hardcoded Color Literals in LoudnessPanel.cpp

## Description
`LoudnessPanel.cpp` contains 5 hardcoded ARGB hex literals that bypass the `MLIMColours` system. These were missed in the Task 169 colour consolidation pass.

**Occurrences**:
1. `0xff2A2A2A` (line ~12) — `resetButton_` background. Maps to `MLIMColours::widgetBackground`.
2. `0xff1E1E2A` (line ~24) — `targetButton_` background (dark navy tint). No exact match; add `MLIMColours::accentDarkBackground { 0xff1E1E2A }` (dark background with blue-navy tint for accent buttons).
3. `0xff2A2A3A` (line ~302) — target level histogram row highlight (slightly lighter navy). Add `MLIMColours::histogramHighlight { 0xff2A2A3A }` in the "Meter / loudness" section.
4. `0xffFF8C00` (line ~385) — "at target ±2 LU" orange colour in `getHistogramBarColour()`. Add `MLIMColours::meterAtTarget { 0xffFF8C00 }` in the "Meter colours" section.
5. `0xff222222` (line ~456) — bar track background fill. Very close to `MLIMColours::background` (0xff1E1E1E); add `MLIMColours::barTrackBackground { 0xff222222 }` or map to `background` if the difference is imperceptible.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/Colours.h` — check existing constants before adding new ones
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — replace 5 hex literals with named constants
Modify: `M-LIM/src/ui/Colours.h` — add new named constants for values with no existing match

## Acceptance Criteria
- [ ] Run: `grep -n "0xff[0-9A-Fa-f]\{6\}" M-LIM/src/ui/LoudnessPanel.cpp` → Expected: zero results
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (colour alias change only)

## Technical Details
New constants to add to `Colours.h`:
```cpp
// In the Meter/Loudness section:
const juce::Colour meterAtTarget        { 0xffFF8C00 };  // orange — at target ±2 LU
const juce::Colour histogramHighlight   { 0xff2A2A3A };  // target level row highlight in histogram

// In the Background colours section:
const juce::Colour accentDarkBackground { 0xff1E1E2A };  // dark navy tint for accent buttons
const juce::Colour barTrackBackground   { 0xff222222 };  // background track behind progress bars
```

Existing mappings (no new constant needed):
- `0xff2A2A2A` → `MLIMColours::widgetBackground` (already defined)

## Dependencies
None
