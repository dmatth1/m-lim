# Task 458: Input Gain Badge — Style to Match Pro-L 2 "Gain" Overlay

## Description
Pro-L 2 shows a floating "Gain" knob overlay in the bottom-left of the waveform area with a small knob icon and value display. M-LIM shows a "+0.0" text badge with a dark background. While functionally equivalent, the visual styling differs:

Pro-L 2 "Gain" overlay (visible in v1-0020.png, v1-0030.png, prol2-intro.jpg):
- Shows "GAIN" label text above
- Value text below (e.g., "+10.5")
- Rounded rectangle background with slight transparency
- Uses the same font/color scheme as other controls
- Background is dark semi-transparent (similar to `widgetBackground` with alpha)

M-LIM's badge (in PluginEditor.cpp):
- 80x20px badge
- Shows "+0.0" value text
- Dark background

**Fix approach**: Add "GAIN" label text above the value in the badge. Increase badge height to ~30px to accommodate both label and value. Use `textSecondary` for the "GAIN" label and `textPrimary` for the value. Add slight rounded corners (3px radius) and use `widgetBackground.withAlpha(0.85f)` for background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — update the input gain badge drawing code to include "GAIN" label and adjust dimensions
Read: `src/PluginEditor.h` — badge positioning constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "gain-badge.png" && stop_app` → Expected: badge shows "GAIN" label above value

## Tests
None

## Technical Details
The badge is drawn in `PluginEditor::paint()` as an overlay in the waveform area. The current badge size is 80x20px positioned at waveform bottom-left. Increase to 80x30px and split into two text rows: "GAIN" (9pt, textSecondary) and value (10pt, textPrimary).

## Dependencies
None
