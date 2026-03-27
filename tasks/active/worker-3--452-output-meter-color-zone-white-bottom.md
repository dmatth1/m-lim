# Task 452: Output Meter Active Fill — Add White/Light-Blue Bottom Zone

## Description
In Pro-L 2 reference screenshots (prol2-main-ui.jpg, prol2-intro.jpg, prol2-metering.jpg), the output level meter bars show a distinctive color progression from bottom to top:
1. **White/bright** at the very bottom (-60 to -40 dBFS)
2. **Light blue** (-40 to -18 dBFS)
3. **Yellow/amber** (-18 to -3 dBFS)
4. **Orange** (-3 to -0.5 dBFS)
5. **Red** (above -0.5 dBFS)

M-LIM's current gradient in `LevelMeter.cpp` `drawChannel()` lines 138-143 goes:
- `meterDanger` (red) at top → `meterWarning` (yellow) at -3 dB → `grMeterLow` (amber) at warning → `meterSafe` (steel blue) at bottom

The bottom zone lacks the white/bright appearance visible in Pro-L 2. The meter fills should transition from the steel-blue `meterSafe` through a brighter white-ish blue near the very bottom.

**Fix approach**: Add a gradient stop near the bottom of the bar (at ~85-90% from top, around -27 to -30 dBFS) using a brighter color like `meterSafe.brighter(0.6f)` to create the white-bottom appearance visible in Pro-L 2.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — in `drawChannel()`, add a gradient stop near the bottom for brighter meter fill (around lines 138-143)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
In the active fill gradient (lines 138-143), add a stop at position ~0.85 (normalized from top) with `meterSafe.brighter(0.5f)` to create the bright bottom zone. The final stop at 1.0 can remain as `meterSafe.darker(0.1f)` or transition to the brighter color.

Reference screenshots: prol2-main-ui.jpg, prol2-metering.jpg clearly show white/bright segments at the bottom of the output meter bars.

## Dependencies
Requires task 451
