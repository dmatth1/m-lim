# Task: Tune Output Meter and Right Zone Background to Reduce Right Zone RMSE

## Description
The Right zone (x=720–900, 180px wide) has RMSE of 23.50% — the second-highest zone error.
This zone contains the output level meter (100px) and the right portion of the loudness panel (80px).

Right zone pixel averages:
- M-LIM: R=61, G=60, B=70 (cool dark, near-idle meters)
- Reference: R=81, G=75, B=74 (warmer, brighter — active audio streaming curve visible)

The reference right zone shows:
1. A distinctive "streaming curve" — a warm orange/amber flowing curve through the meter area
   (the loudness history visualisation, prominent in the Pro-L 2 screenshot)
2. The large "-13.2 LUFS" readout in orange (#E87828-like) at the bottom right
3. Wide level meter bars showing active audio (warm yellow/orange lower half)

M-LIM's right zone at idle shows dark idle meter segments and smaller text readouts.

The most impactful colour improvement is the `loudnessPanelBackground` in the right zone.
Current value: 0xff464356 (R=70, G=67, B=86).
Measurement at x=720–760 (top half, panel background, no text): M-LIM R=67, G=63, B=80.
Reference at same zone (top-half panel area): R=26, G=23, B=29 (very dark — reference panel
top area IS dark, darker than M-LIM).

Further investigation: the right zone difference is largely content-driven (active audio).
Structural improvements — ensuring the `drawLargeReadout` in `LoudnessPanel.cpp` renders
the LUFS value at a font size matching the reference (~36–42pt, centred) — would help when
audio is playing.

For this task: verify the large LUFS readout function in `LoudnessPanel.cpp::drawLargeReadout`
and ensure:
1. Font size is ≥ 32pt (matches the prominent "-13.2" in the reference)
2. The colour used is `lufsReadoutGood` (0xffE8C040 warm yellow) or an orange variant
   that better matches the reference's orange "-13.2"
3. The readout area is positioned at the bottom of the panel, occupying at least 40px height

## Produces
None

## Consumes
None

## Relevant Files
Read:   `src/ui/LoudnessPanel.cpp` — find drawLargeReadout, check font size and colour
Read:   `src/ui/LoudnessPanel.h` — check component height constraints
Modify: `src/ui/LoudnessPanel.cpp` — increase large readout font size if < 32pt
Modify: `src/ui/Colours.h` — consider adjusting lufsReadoutGood from 0xffE8C040 toward
          a warmer orange (0xffE87828) to match the reference's "-13.2" orange colour

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

RMSE zones:
```bash
convert /tmp/task-mlim.png -crop 180x500+720+0 +repage /tmp/cur-right.png
convert /tmp/task-ref.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/ref-right.png /tmp/cur-right.png /dev/null 2>&1
```

## Dependencies
None
