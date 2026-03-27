# Task 460: Waveform dB Grid Labels — Remove "dB" Suffix to Match Pro-L 2

## Description
The waveform display's horizontal grid lines show labels like "-3 dB", "-6 dB", "-9 dB" etc. In Pro-L 2 reference screenshots (prol2-main-ui.jpg, v1-0040.png), the waveform dB labels show just the number with "dB" suffix: "-3 dB", "-6 dB". However, comparing closely, the Pro-L 2 labels are more compact and use a smaller font. The M-LIM labels appear too large and prominent compared to the subtle Pro-L 2 grid text.

Looking at the reference more carefully, the Pro-L 2 labels are:
- Positioned on the LEFT side of the waveform area (not inside the waveform)
- Very small, subtle gray text
- Format: "-3 dB", "-6 dB", etc. (with "dB" suffix, matching M-LIM)

The actual issue is that M-LIM's dB labels are too prominent — they need to be slightly smaller and more transparent to match the subtle appearance in Pro-L 2.

**Fix approach**: Reduce the grid label font size to `kFontSizeSmall - 1.0f` (8pt) and use `textSecondary.withAlpha(0.55f)` instead of full `textSecondary` opacity. Also reduce grid line alpha from 0.35 to 0.25.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — in `drawScale()` function: reduce font size and alpha of dB labels; in `drawBackground()`: reduce grid line alpha

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "grid-labels.png" && stop_app` → Expected: grid labels are subtler and less prominent

## Tests
None

## Technical Details
In `WaveformDisplay::drawScale()`, the labels are drawn with `MLIMColours::textSecondary` at full opacity. Reducing to `withAlpha(0.55f)` and using a smaller font will make them more subtle.
In `drawBackground()`, the grid lines use `waveformGridLine.withAlpha(0.35f)` — reducing to 0.25 makes them more subtle.

## Dependencies
None
