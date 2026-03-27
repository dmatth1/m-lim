# Task 355: Widen Output Level Meter and Move to Rightmost Position

## Description

The right panel sub-region (100x400+800+50) has the highest RMSE at 29.59%. This crop
captures x=800–900, which currently shows the LoudnessPanel (dark text-heavy panel,
avg colour ~#2D2A2C ≈ 17% brightness). The Pro-L 2 reference shows colorful level-meter
bars in that same region (avg ~#514A48 ≈ 30% brightness).

Currently in PluginEditor.cpp `resized()` the order is:
1. LoudnessPanel removed first → rightmost (x=760–900)
2. OutputMeter removed second → x=702–760 (58 px wide, NOT in the crop)

Fix: swap the removal order AND widen OutputMeter so it occupies x=800–900, making
the right panel crop capture the level meter bars instead of the text-only panel.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.h` — change `kOutputMeterW = 58` → `kOutputMeterW = 100`
Modify: `src/PluginEditor.cpp` — in `resized()`, swap the two `removeFromRight` calls so
  `outputMeter_` is removed first (rightmost), then `loudnessPanel_`

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: screenshot methodology (scrot 1920x1080, crop 908x500+509+325, resize 900x500!) then
  `compare -metric RMSE mlim.png ref.png diff.png 2>&1` →
  Expected: right panel sub-region (100x400+800+50) RMSE < 28.00% (improvement from 29.59%)
- [ ] Run: identify that the OutputMeter component occupies the rightmost ~100px of the plugin
  (visually verify the level meter bars appear at far right edge)

## Tests
None

## Technical Details

In `src/PluginEditor.h`, change:
```cpp
static constexpr int kOutputMeterW   = 58;
```
to:
```cpp
static constexpr int kOutputMeterW   = 100;
```

In `src/PluginEditor.cpp` `resized()`, change the removal order from:
```cpp
loudnessPanel_.setBounds (bounds.removeFromRight (kLoudnessPanelW));
outputMeter_.setBounds   (bounds.removeFromRight (kOutputMeterW));
```
to:
```cpp
outputMeter_.setBounds   (bounds.removeFromRight (kOutputMeterW));    // rightmost
loudnessPanel_.setBounds (bounds.removeFromRight (kLoudnessPanelW));
```

With these changes, layout from right becomes:
- OutputMeter: x=800–900 (100 px, rightmost)
- LoudnessPanel: x=660–800 (140 px)
- GRMeter: x=648–660 (12 px)
- WaveformDisplay: x=0–648

The right panel RMSE crop (100x400+800+50) will then capture the OutputMeter bars rather
than the LoudnessPanel text.

**RMSE measurement methodology** (use CORRECT crop — see task 354 for details):
```bash
# Reference crop (crop first, then resize)
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-355-ref.png

# M-LIM: scrot 1920x1080, crop app area, resize
# (use Scripts/ui-test-helper.sh start_app methodology)
convert screenshots/task-355-raw.png -crop 908x500+509+325 +repage \
    -resize 900x500! screenshots/task-355-mlim.png

# Sub-region: right panel
compare -metric RMSE \
  <(convert screenshots/task-355-mlim.png -crop 100x400+800+50 +repage png:-) \
  <(convert screenshots/task-355-ref.png  -crop 100x400+800+50 +repage png:-) \
  /dev/null
```

Save full results to `screenshots/task-355-rmse-results.txt`.

## Dependencies
None
