# Task 269: Loudness Panel Background Color Too Dark

## Description
The LoudnessPanel (right side of plugin, holds LUFS readouts + histogram) uses
`MLIMColours::displayBackground = 0xff111118` as its fill color — near-black.
In the reference Pro-L 2, the LUFS/loudness panel area is a medium dark gray
(approximately `#2A2A2A` to `#323232`), noticeably lighter than the waveform
display background. This darkness mismatch is a primary contributor to the
~25.6% RMSE measured for the right panel region in task 264/268.

**Measured reference values (sample from `/reference-docs/reference-screenshots/prol2-main-ui.jpg` right panel area, approximately x=725–870, y=40–340):**
Sample the reference image directly to confirm, but expected range is approximately
`#282828`–`#323232` for the panel fill.

**Fix — two changes:**

1. Add a new colour constant to `src/ui/Colours.h`:
```cpp
const juce::Colour loudnessPanelBackground { 0xff2A2A2A };  // medium dark gray, matches Pro-L 2 loudness panel
```

2. Update `src/ui/LoudnessPanel.cpp` `paint()` (line ~212):
```cpp
// Change from:
g.setColour (MLIMColours::displayBackground);
// Change to:
g.setColour (MLIMColours::loudnessPanelBackground);
```

**Verify the exact target colour** by sampling the reference image at
`/reference-docs/reference-screenshots/prol2-main-ui.jpg` in the right-panel
area before committing. Use:
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1x1+1440+150 +repage txt:- 2>/dev/null | grep -oE '#[0-9A-Fa-f]{6}'
```
(adjust x/y offset until you hit a neutral panel background pixel — avoid knobs, text, borders).
Use the sampled value (or nearest round hex) as `loudnessPanelBackground`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — add `loudnessPanelBackground` constant
Modify: `src/ui/LoudnessPanel.cpp` — update `paint()` to use new constant (~line 212)
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — sample panel color
Read: `screenshots/task-264-rmse-results.txt` — baseline right-panel RMSE

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds with no errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Visual check: launch standalone on Xvfb, screenshot — the right panel (loudness/LUFS area) should appear as medium dark gray, clearly lighter than the near-black waveform display background on the left

## Tests
None (visual color change — no unit tests required)

## Technical Details
`LoudnessPanel::paint()` fills the panel background at line ~212:
```cpp
g.setColour (MLIMColours::displayBackground);
g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
```

Only this one line needs to change (the colour used for `fillRoundedRectangle`).
The border draw immediately after uses `panelBorder` and should remain unchanged.
The histogram, readout rows, and button colours do not need adjustment in this task.

## Dependencies
None
