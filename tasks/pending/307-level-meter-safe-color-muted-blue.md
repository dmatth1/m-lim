# Task 307: Level Meter Safe-Zone Color — Muted Blue-Gray to Match Reference

## Description
The `LevelMeter` currently draws safe-zone segments in a vivid medium blue:
```cpp
const juce::Colour meterSafe { 0xff4D88CC };  // medium cyan-blue
```

The reference Pro-L 2 level meters use a more muted, darker blue-gray for the safe (lower level)
zone. Measurements from reference video frames (v1-0011.png at the safe zone, y≈230) show:
- Reference safe-zone segment colour: approximately `#81818A` = rgb(129, 130, 138)

This is significantly less saturated and darker than M-LIM's `#4D88CC` = rgb(77, 136, 204):
- R: 77 vs 129 (+52 difference)
- G: 136 vs 130 (−6 difference)
- B: 204 vs 138 (−66 difference)

M-LIM's blue is far too saturated and too bright. The reference uses a neutral blue-gray that
sits subtly in the bar rather than standing out visually.

**Fix:** Change `meterSafe` in `Colours.h`:
```cpp
// BEFORE:
const juce::Colour meterSafe { 0xff4D88CC };

// AFTER:
const juce::Colour meterSafe { 0xff6879A0 };  // muted steel-blue, matches Pro-L 2 safe zone
```

`#6879A0` = rgb(104, 121, 160) — a dark desaturated blue-gray that closely approximates the
reference meter bar appearance while retaining the blue family colour identity.

This affects both the left level meters (currently 26% RMSE) and the output meter in the right
panel. When audio is present and the meter bar fills through the safe zone, it will visually
match the Pro-L 2 reference much more closely.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `meterSafe` colour constant
Read: `M-LIM/src/ui/LevelMeter.cpp` — verify `meterSafe` is used in `drawChannel()` safe zone
Read: `/reference-docs/video-frames/v1-0011.png` — reference safe-zone colour evidence

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: Launch standalone. Confirm level meter bars in the safe zone appear as a muted dark
  blue-gray (not the vivid bright blue of before). The safe zone should look subtler, more like
  the neutral bar style of the Pro-L 2 reference.
- [ ] Code check: `grep -n "meterSafe\|6879A0\|4D88CC" M-LIM/src/ui/Colours.h` → Expected: shows the updated 0xff6879A0 value.

## Tests
None

## Technical Details
The change is a single-line edit in `Colours.h`:
```cpp
const juce::Colour meterSafe { 0xff6879A0 };  // muted steel-blue (was 0xff4D88CC vivid blue)
```

The `meterWarning` (yellow) and `meterDanger` (red) colours are already appropriate and should
NOT be changed.

## Dependencies
None
