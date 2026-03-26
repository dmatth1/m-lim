# Task 332: Increase Loudness Panel Histogram Area Height

## Description
The right panel RMSE is stuck at 29.66%. A key structural difference between M-LIM and the
reference (prol2-main-ui.jpg) is that the reference's loudness histogram occupies the majority
of the right panel height (~330px), while M-LIM's histogram is only 164px tall (the readout
rows take up 204px of the 368px panel).

In the reference, the LUFS readouts area is compact (~50-60px), and the histogram takes up
the remaining ~270-300px. M-LIM has five text rows (Momentary, Short-Term, Integrated, Range,
True Peak) plus a large readout and button row, totalling 204px.

To match the reference structure:
1. Make the readout rows more compact (reduce kRowH and/or reduce count)
2. Or: reduce kLargeReadoutH
3. Or: reduce kMeasureBtnRowH
4. Combine changes to get kReadoutAreaH closer to ~140px (from current 204px)

If kReadoutAreaH is reduced to ~140px, the histogram height increases from 164px to ~228px,
which would better represent the reference's proportionally large histogram area.

Measured kReadoutAreaH in LoudnessPanel.h:
  kReadoutAreaH = kPadding(4) + 5*kRowH(22) + kPadding(4) + kLargeReadoutH(62) + kMeasureBtnRowH(24) = 204

Target: reduce to ~140-150px by:
- Reduce kRowH from 22 to 18 (saves 20px)
- Reduce kLargeReadoutH from 62 to 48 (saves 14px)
- Reduce kMeasureBtnRowH from 24 to 20 (saves 4px)
- Total savings: 38px → new kReadoutAreaH ≈ 166px, histogram ≈ 202px

This gives a 23% taller histogram area.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.h` — reduce kRowH, kLargeReadoutH, kMeasureBtnRowH
Read: `M-LIM/src/ui/LoudnessPanel.cpp` — verify paint() and resized() still work correctly after changes

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-332-after.png" && stop_app` → Expected: screenshot saved
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
4. The "LUFS" and "Short Term" / "Momentary" display buttons are readable

Worker: If the changes cause layout issues (buttons overlapping text, truncated labels),
adjust the row height reductions proportionally to avoid them.

## Dependencies
Requires tasks 328 and 329 (layout reverts) to be complete first so that panel height is
correct (368px at kControlStripH=92, not 368px at kControlStripH=108).

Wait — with kControlStripH=92 (after task 329), the panel height = 500-24-92 = 384px, not 368px.
With kControlStripH=108 (current), panel height = 500-24-108 = 368px.

So after task 329 is applied: panel height becomes 384px.
Current kReadoutAreaH = 204px → histogram = 384-204 = 180px (taller already).
With proposed changes: kReadoutAreaH ≈ 166px → histogram = 384-166 = 218px.

Adjust targets accordingly if task 329 is merged before this task.
