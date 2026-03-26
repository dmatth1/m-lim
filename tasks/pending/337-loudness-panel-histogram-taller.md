# Task 337: Increase Loudness Panel Histogram Area Height

## Description
The right panel RMSE is stuck at ~29%. A key structural difference between M-LIM and the
reference (prol2-main-ui.jpg) is that the reference's loudness histogram occupies the majority
of the right panel height (~330px), while M-LIM's histogram is much shorter because the readout
rows take up 204px of the panel.

In the reference, the LUFS readouts area is compact (~50-60px), and the histogram takes up
the remaining ~270-300px. M-LIM has five text rows (Momentary, Short-Term, Integrated, Range,
True Peak) plus a large readout and button row, totalling 204px.

To match the reference structure, make the readout rows more compact:
- Reduce kRowH from 22 to 18 (saves 20px)
- Reduce kLargeReadoutH from 62 to 48 (saves 14px)
- Reduce kMeasureBtnRowH from 24 to 20 (saves 4px)
- Total savings: 38px → new kReadoutAreaH ≈ 166px

With kControlStripH=92 (already applied via done task 328), panel height = 500-24-92 = 384px.
- Current kReadoutAreaH = 204px → histogram = 384-204 = 180px
- After changes: kReadoutAreaH ≈ 166px → histogram = 384-166 = 218px

This gives a ~21% taller histogram area.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.h` — reduce kRowH, kLargeReadoutH, kMeasureBtnRowH
Read: `M-LIM/src/ui/LoudnessPanel.cpp` — verify paint() and resized() still work correctly

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-337-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: Loudness histogram area is noticeably taller; LUFS readout rows and large readout are more compact but still readable
- [ ] Visual: No layout overlap or truncated text

## Tests
None

## Technical Details
In `M-LIM/src/ui/LoudnessPanel.h`:

```cpp
// BEFORE:
static constexpr int kRowH          = 22;
static constexpr int kLargeReadoutH = 62;
static constexpr int kMeasureBtnRowH = 24;

// AFTER:
static constexpr int kRowH          = 18;
static constexpr int kLargeReadoutH = 48;
static constexpr int kMeasureBtnRowH = 20;
```

After making changes, check that:
1. `LoudnessPanel::resized()` correctly positions buttons (resetButton_, targetButton_,
   pauseMeasurementButton_, measurementModeButton_) — these use kRowH and kLargeReadoutH
2. `LoudnessPanel::paint()` has no overlap between histogram area and readout rows
3. The "RST" button and target selector button are still positioned correctly
4. All label text is readable at the reduced sizes

Worker: If the changes cause layout issues (buttons overlapping text, truncated labels),
adjust the row height reductions proportionally to avoid them.

## Dependencies
None (tasks 328 and 329 layout reverts are already done)
