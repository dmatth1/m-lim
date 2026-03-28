# Task: Darken Output Meter Idle Fill to Near-Black (Match Reference)

## Description
The output meter bars show a consistently bright idle fill (~srgb(60,58,73) at Y=250-400) when no audio is playing. The Pro-L 2 reference output meter at the same positions shows near-black values (~srgb(24,22,25) to srgb(33,28,34)). The current idle brightness is 2-3x too high.

The issue is in `LevelMeter.cpp`'s idle structural gradient. When `showScale_` is false (output meter), the `aScale` multiplier is 0.18, but the bar track background (`barTrackBackground` = 0xff2A2838 = (42,40,56)) and the meter background gradient (`meterSafe.withAlpha(0.12f)`) still produce a combined brightness much higher than the reference.

Pixel comparison at x=850:
- Y=200: current srgb(55,54,66), reference srgb(56,56,69) — close at top!
- Y=250: current srgb(60,58,73), reference srgb(24,22,25) — 36 units too bright
- Y=350: current srgb(60,58,73), reference srgb(28,26,31) — 32 units too bright

The output meter should show near-black idle state below the active level, with only the active level region illuminated.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — lines 86-117: idle structural gradient for output meter (showScale_=false path)
Modify: `src/ui/Colours.h` — `barTrackBackground` constant may need a darker variant for output meters
Read: `src/PluginEditor.cpp` — output meter configuration (setIdleSimulationLevel, setShowScale)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot comparison → Expected: output meter idle bars are near-black (RGB ~25-35), matching reference

## Tests
None

## Technical Details
- The output meter uses `setIdleSimulationLevel(-18.0f)` and `setShowScale(false)`
- The `aScale` = 0.18f for output meters (line 98), but this just reduces gradient alpha — the `barTrackBackground` fill (line 76-77) is always drawn full-opacity
- Fix approach: for output meters (showScale_=false), use a much darker bar track background — either `displayBackground` (0xff111118) or a new near-black constant (~0xff181820)
- Alternatively, reduce the bg gradient alpha further for output meters (lines 238-246), lowering `bgAlphaTop`/`bgAlphaBot` to near-zero
- The output meter's separate LED segment separators (line 82-84) also add brightness — these should use a darker separator color for output meters
- Current `barTrackBackground.brighter(0.25f)` is used for separators — for output meters this should be `barTrackBackground.darker(0.3f)` or similar

## Dependencies
None
