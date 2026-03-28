# Task: Input Meter Upper Zone (Top 30%) Too Bright — Should Be Near-Black

## Description
The input meter upper portion (Y=80-200) shows idle fill that is far too bright compared to the reference:

- Y=80: current srgb(66,62,72), reference srgb(37,31,37) — 30 units too bright
- Y=120: current srgb(124,108,60), reference srgb(29,27,32) — 80+ units too bright (yellow idle fill visible!)
- Y=160: current srgb(107,107,112), reference srgb(24,22,25) — 83 units too bright
- Y=200: current srgb(107,107,112), reference srgb(24,22,25) — 83 units too bright

The reference shows the top ~40% of the input meter as near-black (no audio present above about -6 dBFS). Our idle simulation gradient extends too high and is too bright.

The yellow tint at Y=120 (srgb(124,108,60)) suggests the warning/danger zone idle color bleeds into the upper meter even when there's no signal, which is wrong — in the reference, only the bottom portion of the meter shows active blue fills.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — idle structural gradient (lines 86-117); the `idleSimLevel_` (-6 dBFS for input) controls where the idle fill starts, but the gradient extends beyond that
Modify: `src/ui/Colours.h` — idle fill colors and meter background

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot comparison → Expected: top 40% of input meter is near-black (< RGB 40)

## Tests
None

## Technical Details
- The idle gradient at line 100-113 uses `meterDanger.withAlpha(0.65f * aScale)` at the simulated peak position, then transitions through yellow/warm to safe
- With `idleSimLevel_ = -6.0f` and `aScale = 1.0f` for input meters, the danger/warning colors bleed into the upper portion
- The idle fill ABOVE the simulated level position (Y < simFillTop) should have no visible color — only the bar track background
- The gradient starting point at `barTop` (line 101) means color extends above `simFillTop` through the gradient interpolation
- Fix: restructure the idle gradient so it only fills below `simFillTop`, leaving the upper area as pure `barTrackBackground`
- For the area above the simulated fill, render only the near-black track background (~srgb(25,23,28))

## Dependencies
None
