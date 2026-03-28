# Task: Add Blue Tint to Input Meter Idle Appearance

## Description
The input meter (left-side vertical bar) idle fill is too neutral/gray compared to Pro-L 2 reference. Pixel comparison at input meter bar mid-height: M-LIM renders ~(64,65,68) — nearly achromatic — while reference shows ~(72,82,114) with a strong blue tint (B channel 46 units higher than R). The input meter's idle gradient should have more blue saturation to match the reference's blue-tinted appearance.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — idle gradient fill for the input meter (which has `showScale_ = true`); the gradient uses `meterSafe` color but renders too neutral
Modify: `src/ui/Colours.h` — `meterSafe` (0xff81828A = 129,130,138) has minimal blue bias; consider shifting to ~(100,115,155) for stronger blue tint
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference input meter has visible blue tint

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at input meter mid-height → Expected: visible blue tint, B channel significantly higher than R

## Tests
None

## Technical Details
- Current `meterSafe`: `0xff81828A` = RGB(129,130,138) — B is only 9 units above R, nearly neutral gray
- Reference input meter at idle: ~(72,82,114) — B is 42 units above R, strong blue tint
- The idle gradient composites `meterSafe` at various alphas over `barTrackBackground`
- Options: (a) increase the B channel in `meterSafe` to ~155, producing RGB(100,115,155), or (b) add a separate blue-tinted idle fill layer in LevelMeter.cpp for the input meter
- Note: `meterSafe` is also used for active meter fill in the safe zone; changing it will affect the active meter color too. This may be desirable (Pro-L 2's safe zone is also blue-tinted) or may need a separate idle color.
- Coordinate with the existing grooming task "brighten-input-meter-idle-gradient.md" which addresses brightness but not saturation.

## Dependencies
None
