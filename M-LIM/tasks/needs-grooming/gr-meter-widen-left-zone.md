# Task: Widen GR Meter to Improve Left Zone Visual Parity

## Description
The GainReductionMeter is currently only 12px wide (`kGRMeterW=12` in `PluginEditor.h`).
The scale labels are disabled (`kScaleW=0` in `GainReductionMeter.h`).

In the reference Pro-L 2, the equivalent display zone (x=640–720 of the 900x500 crop,
the "Left zone") shows a wide, segmented stereo level meter bar. M-LIM's 12px GR bar is
nearly invisible and contributes to the Left zone RMSE being 26.22%.

Left zone pixel averages (x=640–720, y=20–420):
- M-LIM: R=63, G=58, B=66  (very dark from 12px GR meter + LoudnessPanel)
- Reference: R=95, G=97, B=117 (bright from active stereo level meter)

Increase `kGRMeterW` from 12 to 28 px and reduce `kLoudnessPanelW` from 140 to 124 px to
compensate. Also enable scale labels on the GR meter (`kScaleW` in `GainReductionMeter.h`
from 0 to 16) to show dB values matching the reference's dB scale.

Separately, the `barTrackBackground` color (0xff231417 = R=35, G=20, B=23) used for the GR
bar track is very dark/warm-red. The reference left zone idle background is a medium dark
blue-grey. Consider lightening `barTrackBackground` to something like 0xff2A2530 (R=42,
G=37, B=48) to add blue tint and reduce the warm-red cast that diverges from reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.h` — change kGRMeterW from 12 to 28, kLoudnessPanelW from 140 to 124
Modify: `src/ui/GainReductionMeter.h` — change kScaleW from 0 to 16 to show dB labels
Modify: `src/ui/Colours.h` — consider adjusting barTrackBackground for less warm-red cast
Read:   `src/PluginEditor.cpp` — confirm resized() layout is not hardcoded elsewhere
Read:   `src/ui/GainReductionMeter.cpp` — confirm drawScale renders well at 16px width

## Acceptance Criteria
- [ ] Run RMSE methodology → Expected: Left zone RMSE ≤ 26.22% (wave-22 baseline)
- [ ] Run RMSE methodology → Expected: Full RMSE ≤ 19.11% (wave-22 full baseline)
- [ ] Visual: GR bar is clearly visible at 28px width with dB scale labels

## Tests
None

## Technical Details
Current layout from `PluginEditor.cpp::resized()`:
```cpp
outputMeter_.setBounds   (bounds.removeFromRight (kOutputMeterW));    // 100px
loudnessPanel_.setBounds (bounds.removeFromRight (kLoudnessPanelW));  // 140px
grMeter_.setBounds       (bounds.removeFromRight (kGRMeterW));         // 12px
waveformDisplay_.setBounds (bounds);                                   // 648px remaining
```

After change: grMeter_=28px, loudnessPanel_=124px, total right panels=252px (unchanged).
The waveform width remains 648px — no change to Wave zone RMSE expected.

RMSE methodology — use the same 5-zone measurement as wave-22:
```bash
convert /tmp/task-mlim.png -crop 80x500+640+0  +repage /tmp/cur-left.png
convert /tmp/task-ref.png  -crop 80x500+640+0  +repage /tmp/ref-left.png
compare -metric RMSE /tmp/ref-left.png /tmp/cur-left.png /dev/null 2>&1
```

## Dependencies
None
