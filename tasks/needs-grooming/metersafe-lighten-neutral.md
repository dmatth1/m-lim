# Task: Lighten meterSafe to Match Reference Safe-Zone Fill Color

## Description
The `meterSafe` color constant in `Colours.h` (#6879A0 = RGB 104, 121, 160) is too dark and too blue-saturated compared to the reference Pro-L 2 safe-zone fill color measured as approximately RGB(129, 130, 138). This color is used in:
1. The active level meter fill gradient at the safe zone (position 0.85 in gradient)
2. The idle structural gradient for level meters
3. LoudnessPanel histogram bars

The reference value was measured from the Pro-L 2 reference image at the safe zone of the input level meter (y≈230, safe zone fill with active audio).

**Change**: Update `meterSafe` from `0xff6879A0` to `0xff81828A` (RGB 129, 130, 138 — lighter neutral gray-blue).

Also update the idle gradient bottom stop alpha in `LevelMeter.cpp::drawChannel()` from `meterSafe.withAlpha(0.80f)` to `meterSafe.withAlpha(1.0f)` so that the composite over `barTrackBackground` (#2A2838) equals the target color:
- With alpha 1.0: composite = (129, 130, 138) exactly matching reference

Additionally, update the mid-range idle gradient stops to be proportionally brighter:
- normTrans (position 0.30): change from alpha 0.30 → 0.55
- normMidFill (position 0.40): change from alpha 0.18 → 0.35

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `meterSafe` constant
Modify: `src/ui/LevelMeter.cpp` — update idle gradient alpha values at bottom stop and mid-range stops
Read: `src/ui/LoudnessPanel.cpp` — histogram bar colours use meterSafe; verify they still look appropriate with lighter value

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: build succeeds, exit 0
- [ ] Run: screenshot captured and compared → Expected: left meter zone shows lighter, more neutral color closer to RGB(129, 130, 138) at the bottom

## Tests
None

## Technical Details
- `meterSafe` new value: `0xff81828A`
- LevelMeter.cpp idle gradient bottom stop: `meterSafe.withAlpha(1.0f)` (was `0.80f`)
- LevelMeter.cpp `normTrans` stop: `meterSafe.withAlpha(0.55f)` (was `0.30f`)
- LevelMeter.cpp `normMidFill` stop: `meterSafe.withAlpha(0.35f)` (was `0.18f`)
- LevelMeter.cpp `normWarmExt` stop: keep as `meterSafe.withAlpha(0.10f)` or increase to `0.15f`
- Expected RMSE improvement: ~1.5-2.5pp for Left zone (currently 26.22%), ~0.5pp for Full (currently 19.07%)

## Dependencies
None
