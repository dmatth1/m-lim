# Task: Input Meter Should Show Waveform-Overlapping Blue-Purple Appearance

## Description
The input meter (left edge, 30px wide) shows a uniform medium-gray idle fill (~srgb(107,107,112) from Y=160-400), but the Pro-L 2 reference shows a gradient of blue-purple active bars in this area matching the waveform content:
- Y=250: reference srgb(80,98,149) — strong blue
- Y=300: reference srgb(94,101,129) — blue-gray
- Y=350: reference srgb(135,144,173) — lighter blue-gray
- Y=400: reference srgb(122,128,157) — medium blue-gray

The reference input meter is not a separate isolated meter with gray bars — it's visually integrated with the waveform, showing the same blue/purple color palette as the waveform area. The current uniform gray appearance (107,107,112) has no blue tint and is too monotone.

The input meter idle fill should use blues and purples that match the waveform's idle gradient, with brightness increasing toward the bottom (mirroring the waveform's gradient).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — idle gradient colors for input meter (showScale_=true path, lines 86-117)
Modify: `src/ui/Colours.h` — `barTrackBackground`, `meterSafe` colors need blue shift for input meter
Read: `src/PluginEditor.cpp` — input meter configuration

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot comparison → Expected: input meter idle bars show blue-purple gradient matching waveform colors, not uniform gray

## Tests
None

## Technical Details
- The idle gradient in LevelMeter uses `meterSafe` (0xff81828A = 129,130,138) which is gray, not blue
- Reference colors suggest the input meter should use colors closer to `inputWaveform` (0xCC6878A0) or a dedicated blue-purple meter idle color
- The gradient should vary vertically: darker/bluer at top (~80,98,149), lighter blue-gray at bottom (~135,144,173)
- Consider adding a vertical blue gradient to the idle fill that mirrors the waveform display's gradient
- The `barTrackBackground` for input meters should have more blue saturation: change from (42,40,56) to something like (50,60,90)
- This task overlaps with "add-input-meter-blue-tint" and "brighten-input-meter-idle-gradient" — these should be coordinated

## Dependencies
None
