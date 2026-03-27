# Task 418: Tune Output Meter and Loudness Panel to Reduce Right Zone RMSE

## Description
The Right zone (x=720–900, 180px wide) has RMSE of 23.50% — the second-highest zone error.
This zone contains the output level meter (100px) and right portion of the loudness panel (80px).

Right zone pixel averages:
- M-LIM: R=61, G=60, B=70 (cool dark, near-idle meters)
- Reference: R=81, G=75, B=74 (warmer, brighter — active audio streaming curve visible)

The reference right zone shows:
1. A large "-13.2 LUFS" readout in orange (#E87828-like) at the bottom right
2. Wide level meter bars showing active audio (warm yellow/orange lower half)
3. A streaming curve (loudness history) visible in warm amber through the meter area

The most impactful structural improvement: ensure `drawLargeReadout` in `LoudnessPanel.cpp`
renders the LUFS value at a font size matching the reference (~36–42pt, centred) and with
a more orange colour. Also consider shifting `lufsReadoutGood` from yellow toward orange.

## Produces
None

## Consumes
None

## Relevant Files
Read:   `src/ui/LoudnessPanel.cpp` — find drawLargeReadout, check font size and colour
Read:   `src/ui/LoudnessPanel.h` — check component height constraints
Modify: `src/ui/LoudnessPanel.cpp` — increase large readout font size if < 32pt
Modify: `src/ui/Colours.h` — adjust lufsReadoutGood from 0xffE8C040 toward 0xffE87828 (warmer orange)

## Acceptance Criteria
- [ ] Run RMSE methodology → Expected: Right zone RMSE ≤ 23.50% (wave-22 baseline)
- [ ] Run RMSE methodology → Expected: Full RMSE ≤ 19.11% (wave-22 full baseline)
- [ ] Visual: large LUFS number visible at bottom-right when audio is playing

## Tests
None

## Technical Details
Reference "-13.2" colour: approximately RGB(232, 120, 40) = 0xffE87828 (warm orange).
Current `lufsReadoutGood`: 0xffE8C040 (warm yellow — too green/yellow vs orange reference).

In `LoudnessPanel.cpp`, search for `drawLargeReadout` and check the `setFont` call.
If font size < 32.0f, increase to 36.0f. If font < 32pt the number is too small relative
to the reference's large "−13.2" display.

Proposed colour change in `Colours.h`:
```cpp
// Before:
const juce::Colour lufsReadoutGood { 0xffE8C040 };  // yellow
// After:
const juce::Colour lufsReadoutGood { 0xffE87828 };  // warm orange — matches reference
```

RMSE zones:
```bash
convert /tmp/task-mlim.png -crop 180x500+720+0 +repage /tmp/cur-right.png
convert /tmp/task-ref.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/ref-right.png /tmp/cur-right.png /dev/null 2>&1
```

## Dependencies
None
