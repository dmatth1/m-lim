# Task 280: Control Strip Height Reduction (120 → 92 px)

## Description
Pro-L 2's control strip is significantly more compact than M-LIM's current 120 px.
Comparing the reference (prol2-features.jpg, v1-0016, v1-0018):
- Pro-L 2 knob row + labels: approximately 55–60 px
- Pro-L 2 status bar: approximately 20 px
- Total control strip: approximately 75–80 px

M-LIM currently uses `kControlStripH = 120` in PluginEditor.h, and:
- `kKnobLabelH = 12` (space above knob row for section headers)
- `kKnobRowH   = 70` (the knob area itself)
- `kPadding    = 4`
- `kBtnRowH    = 24` (status bar)

Required changes (in ControlStrip.cpp and PluginEditor.h):
1. **PluginEditor.h**: `kControlStripH` 120 → 92
2. **ControlStrip.cpp constants**:
   - `kKnobLabelH`: 12 → 10
   - `kKnobRowH`: 70 → 56
   - `kBtnRowH`: 24 → 20
   - `kPadding`: keep at 4
   - `kOutputLabelH` (if defined): 14 → 12
3. The `RotaryKnob` components must fit in the reduced `kKnobRowH`. Since `RotaryKnob`
   adjusts `knobSize = jmin(width, height - textHeight)`, smaller height just means
   smaller knobs — which actually matches Pro-L 2's more compact appearance.

This change gives 28 more pixels to the waveform area (500 - 30 - 92 = 378 px vs
current 350 px), which increases the visual proportion of the waveform matching Pro-L 2.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.h` — `kControlStripH` constant (line ~64)
Modify: `src/ui/ControlStrip.cpp` — `kKnobLabelH`, `kKnobRowH`, `kBtnRowH`, `kOutputLabelH` in the anonymous namespace (lines ~5–18)
Read: `src/ui/RotaryKnob.cpp` — verify knob sizing adapts to smaller height
Read: `src/ui/ControlStrip.cpp` — `resized()` method to verify layout still correct

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-274-after.png && stop_app` → Expected: screenshot taken
- [ ] Run: `convert screenshots/task-274-after.png -crop 908x1+0+407 +repage /tmp/t274-check.png && identify /tmp/t274-check.png` → Expected: 908×1 strip exists (verifies the control strip sits at the correct reduced height)
- [ ] Visual check: All knobs still visible and correctly labeled; no clipping; control strip is visually more compact than before
- [ ] Run: `compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg screenshots/task-274-after.png 0.15` → Expected: RMSE reported (record result; should be lower than 0.266)

## Tests
None

## Technical Details
Changes summary:
```
PluginEditor.h:
  kControlStripH: 120 → 92

ControlStrip.cpp (anonymous namespace):
  kKnobLabelH:  12 → 10
  kKnobRowH:    70 → 56
  kBtnRowH:     24 → 20
  kOutputLabelH: 14 → 12  (if present)
  kPadding:     4 (unchanged)
```

Verify layout math:
  kKnobLabelH (10) + kKnobRowH (56) + kPadding (4) + kBtnRowH (20) = 90 ≤ 92 ✓

The output ceiling slider `resized()` spans `totalH = kKnobRowH + kPadding + kBtnRowH = 80`.
Check that `outputCeilingSlider_.setBounds(rightCol.withHeight(totalH - kOutputLabelH))` still
renders correctly with the new values.

Take before/after screenshots and compare RMSE:
```bash
source Scripts/ui-test-helper.sh
start_app
screenshot screenshots/task-274-before.png
# ... rebuild and restart ...
screenshot screenshots/task-274-after.png
compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  screenshots/task-274-after.png 0.15
stop_app
```

## Dependencies
None (but works well in combination with tasks 272 and 273 which also compact the control strip)
