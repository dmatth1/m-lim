# Task 349: Increase TopBar Height to Reduce Top-Strip RMSE

## Description

The plugin crop's y=20-35 band currently shows the WaveformDisplay gradient top
(~104,99,104) where the reference shows dark Pro-L 2 header background (~43,38,44).
This 16-row band contributes ~0.22% RMSE error at ~60 units/channel error per pixel.

Root cause: `kTopBarH = 24` but the crop starts 4px into the TopBar (the JUCE standalone
window title bar sits above the crop start). The WaveformDisplay starts at crop y=20.
Increasing `kTopBarH` from 24 to 40 pushes WaveformDisplay down by 16px so those rows
show TopBar gradient bottom (~31,31,31) vs reference dark (~43,38,44) → error drops from
60 units/channel to 11 units/channel.

Estimated improvement: ~0.32% full-image RMSE (22.98% → ~22.66%).
Sub-region waveform RMSE impact: negligible (gradient shifts by <2 units/channel at y=50).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — change `kTopBarH` from 24 to 40 (line 64)
Read: `M-LIM/src/PluginEditor.cpp` — verify resized() layout still correct after height change
Read: `M-LIM/src/ui/TopBar.cpp` — TopBar paint() uses gradient from 0xff252525→0xff1F1F1F; with 40px height the gradient fills the taller bar correctly with no code changes needed
Read: `Scripts/ui-test-helper.sh` — for screenshot methodology
Skip: `M-LIM/src/dsp/` — not needed

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-349-after.png" && stop_app` → Expected: screenshot captured
- [ ] Run: `compare -metric RMSE <(convert screenshots/task-349-after.png -crop 900x16+0+20 +repage png:-) <(convert screenshots/audit-ref-crop.png -crop 900x16+0+20 +repage png:-) /dev/null 2>&1` → Expected: RMSE ≤ 0.15 (was 0.219 before this task)
- [ ] Run: sub-region waveform RMSE check: `compare -metric RMSE <(convert screenshots/task-349-after.png -crop 600x400+150+50 +repage png:-) <(convert screenshots/audit-ref-crop.png -crop 600x400+150+50 +repage png:-) /dev/null 2>&1` → Expected: RMSE ≤ 0.205 (no regression from 0.1963 baseline; tolerance +0.01 for minor gradient shift)

## Tests
None

## Technical Details

In `M-LIM/src/PluginEditor.h`:
```cpp
static constexpr int kTopBarH = 40;  // was 24; increased to reduce waveform-top RMSE
```

No other changes required:
- TopBar's `resized()` uses `getLocalBounds().reduced(4, 2)` — the taller bar gives
  more vertical padding around buttons, which is visually fine (reference has tall header)
- TopBar's `paint()` gradient from 0xff252525→0xff1F1F1F fills the entire 40px correctly
- WaveformDisplay at new y=40 position still fits in layout: 500-40-92=368px content height

Screenshot & RMSE verification:
1. Build: `cmake --build build -j$(nproc)`
2. Run: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-349-after.png" && stop_app`
3. Crop: `convert screenshots/task-349-after.png -crop 908x500+509+325 +repage -geometry 900x500\! screenshots/task-349-crop.png`
   (Note: use the raw scrot output, not the already-cropped file)
4. Compare y=20-35 band: should drop from 0.219 to ~0.10 RMSE
5. Compare waveform sub-region: should remain ≤ 0.205
6. Save results to `screenshots/task-349-rmse-results.txt`

Note: The crop at `+509+325` captures the window starting 4px into the TopBar (y=321 is
where the TopBar starts on screen, crop starts at y=325). With kTopBarH=40, the WaveformDisplay
moves to screen y=361, which is crop y=36. The ceiling line shifts from crop y=20 to y=36.

## Dependencies
None
