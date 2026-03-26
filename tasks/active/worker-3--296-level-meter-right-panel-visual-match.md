# Task 296: Right Panel — Level Meter Bars Wider and More Prominent to Match Reference

## Description
The reference (Pro-L 2) right panel shows two very tall, wide level meter bars with a rich gradient color scheme (bright blue at low levels → yellow → orange → red at loud levels). These bars are visually dominant — they span nearly the full height of the right panel and are clearly visible even in the static screenshot.

Current M-LIM right panel:
- `grMeter_` (50px): Shows GR meter with dark segment texture — distinct from reference level bar style
- `outputMeter_` (38px): Very narrow bars, dark segments — does not match the bright, wide reference bars

The right-panel RMSE is currently 22.78%. The reference shows the level meters as the dominant visual element of the right panel, while M-LIM shows a relatively featureless GR scale + dark narrow output bars.

**Investigation areas (read before implementing):**
1. In `src/ui/LevelMeter.cpp`: check `kBarWidthRatio`, `kGapRatio`, segment height/gap constants
2. In `src/ui/GainReductionMeter.cpp`: check if GR meter bar width/color can be made more reference-like
3. In `src/ui/Colours.h`: check `meterGreen`, `meterYellow`, `meterOrange`, `meterRed` values against reference

**Expected changes:**
- Increase `kBarWidthRatio` in `LevelMeter.cpp` to make bars wider (currently bars use only a fraction of the 38px width)
- Verify meter segment colors: reference gradient goes from dark blue/teal (quiet) → green → yellow → orange → red (loud). Update color thresholds in `drawChannel()` if they don't match.
- The GR meter (`grMeter_`) should have a similar segment-fill pattern to the reference's thin right column.

**Do NOT** change DSP or parameter logic. Only change rendering constants and colors.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/LevelMeter.cpp` — bar geometry constants, drawChannel color thresholds
Read: `M-LIM/src/ui/LevelMeter.h` — kBarWidthRatio, kGapRatio, kSegH, kSegGap
Read: `M-LIM/src/ui/GainReductionMeter.cpp` — GR meter rendering
Read: `M-LIM/src/ui/Colours.h` — meter color constants
Modify: `M-LIM/src/ui/LevelMeter.cpp` — increase bar width ratio, adjust color thresholds
Modify: `M-LIM/src/ui/Colours.h` — update meter segment colors if needed
Read: `/reference-docs/video-frames/v1-0009.png` — reference level meter appearance

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: Output meter bars visibly wider and fill more of the allocated 38px; color gradient segments visible in the bar.
- [ ] Run RMSE: right-panel crop `200x378+700+30` on 900x500 → Expected: RMSE ≤ 0.21 (improvement from 0.2278)

## Tests
None

## Technical Details
Reference level meters (from v1-0009):
- Two bars side by side, approximately 15-20px each with small gap
- Color gradient: near-black/dark-blue at -∞, stepping through blue → teal → green → yellow → orange → red
- Bright yellow/orange peak hold indicators
- Each bar spans the full display height

## Dependencies
None
