# Task 342: Tune Right Panel (Loudness Panel) Background Color

## Description
The right panel sub-region RMSE is stuck at 31.07% (x=800-900, y=50-450 in 900x500 crop).
This region covers the output level meter and right portion of the loudness panel.

Task 331 darkened the loudness panel background from `0xff2B2729` to `0xff1E1C21`, which
worsened the right panel RMSE from 29.66% (task-324) to 31.07% (task-339).

Analysis of the reference shows:
- The reference right panel region contains an ACTIVE output level meter (bright colored segments)
- The dark panel background areas measure approximately RGB(24-30, 14-20, 18-22) — very dark
- The average brightness of the right panel in the reference is ~34 (gray scale) due to the
  bright meter segments dominating the appearance
- M-LIM at idle shows only the dark `barTrackBackground` (#181818) in the meter area

**Fix**: Revert `loudnessPanelBackground` from `0xff1E1C21` back to `0xff2B2729`.
This brings the panel background slightly closer to the reference's average appearance when
accounting for the bright meter content distribution.

Additionally, explore whether the output level meter's idle track color (`barTrackBackground` =
`0xff181818`) should be slightly lightened to better approximate the reference's average meter
appearance. The reference shows a gradient from very bright (bottom = high level) to dark (top),
so a subtle track brightening toward the bottom of the meter might reduce RMSE.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — revert loudnessPanelBackground
Read: `M-LIM/src/ui/LevelMeter.cpp` — understand idle meter rendering (background track)
Read: `M-LIM/src/PluginEditor.h` — check kOutputMeterW constant

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: visual inspection — right panel background should appear as a slightly lighter
  dark purple-gray (#2B2729) rather than the very dark #1E1C21

## Tests
None

## Technical Details
**Primary fix** — In `M-LIM/src/ui/Colours.h`:
```cpp
// CURRENT (from task 331 — caused regression):
const juce::Colour loudnessPanelBackground { 0xff1E1C21 };

// REVERT TO (pre-task-331 value — gave 29.66% right panel RMSE vs 31.07% now):
const juce::Colour loudnessPanelBackground { 0xff2B2729 };
```

**Optional secondary improvement** — The output meter background track is currently pure dark:
```cpp
const juce::Colour barTrackBackground { 0xff181818 };
```
The reference output meter when active shows many bright segments. If the RMSE does not
improve sufficiently from the primary fix alone, consider adding a subtle vertical gradient
to the LevelMeter's `drawChannel` background instead of the solid `barTrackBackground`:
- Top (silence zone): `barTrackBackground` (~#181818)
- Bottom (loud zone): a slightly lighter color like #1E2535 to suggest the reference meter's
  colorful bottom segments

However, only make this secondary change if the primary fix does not bring right panel RMSE
below 29% (better than pre-task-331 baseline). Do NOT over-engineer — the primary revert is
the priority.

## Dependencies
None (can be done in parallel with tasks 340 and 341)
