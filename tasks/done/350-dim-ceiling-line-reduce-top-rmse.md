# Task 353: Dim waveformCeilingLine — Reduce Top-Area RMSE

## Description

The `waveformCeilingLine` color (0xCCDD4444 — bright red at 80% alpha) renders a prominent
red horizontal band at the very top of the WaveformDisplay at idle (default ceiling = -0.1 dBFS).

**Root cause confirmed by pixel analysis (auditor, 2026-03-27):**

The WaveformDisplay component starts at crop y=20 (plugin y=24). With ceilingDB_ = -0.1f and
kMaxGRdB = 30, the ceiling line is drawn at WaveformDisplay local y ≈ 0–1.5px, which maps to
crop y=20–21.

Pixel values at crop y=21, x=400:
- M-LIM: **#C54A4B** (bright red, 197, 74, 75)
- Reference: **#2B262D** (dark purple, 43, 38, 45)

Per-channel RMSE at those rows ≈ **36%**. The reference shows the FabFilter header background
there — no ceiling line visible. These 2 bright-red rows spanning the full 900px width cause a
notable uplift in the overall full-image RMSE.

**Fix (position only — color already handled by task 350):**

1. **Move the default ceiling position** — change `ceilingDB_` default from -0.1f to **-0.5f**
   in `WaveformDisplay.h`. This shifts the ceiling line ≈6px lower into the waveform body (away
   from the very top edge), so it no longer bleeds into the layout boundary between TopBar and
   waveform.

NOTE: `waveformCeilingLine` color in Colours.h is handled by active task 350 (worker-1).
Do NOT modify Colours.h in this task — only change `ceilingDB_` in WaveformDisplay.h.

This is a small change with no functional impact (the ceiling parameter is still user-adjustable;
only the DEFAULT position changes by 0.4 dBFS).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.h` — change `float ceilingDB_ = -0.1f` to `-0.5f`
Read: `M-LIM/src/ui/WaveformDisplay.cpp` — understand drawCeilingLine() to verify position math
Skip: `M-LIM/src/ui/Colours.h` — waveformCeilingLine color handled by task 350 (do NOT modify)

## Acceptance Criteria
- [ ] Run: `grep 'ceilingDB_' M-LIM/src/ui/WaveformDisplay.h` → Expected: `-0.5f`
- [ ] Run: build succeeds → `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: launch on Xvfb, screenshot, sample pixel at waveform local y≈6 (crop y≈46 after task-349 or y≈26 before) → Expected: ceiling line is 6px below top edge of waveform, not at very top edge

## Tests
None

## Technical Details

**WaveformDisplay.h change (only change needed):**
```cpp
// Line 82 — change default ceiling from -0.1f to -0.5f
float ceilingDB_ = -0.5f;   // output ceiling reference line (dBFS); was -0.1f
```

**Do NOT change Colours.h** — waveformCeilingLine color is handled by task 350.

**Why -0.5f:** At the default display range of 30 dBFS (kMaxGRdB=30):
- frac = 0.5/30 = 0.0167
- y_offset = 0.0167 × (waveform_height ≈ 384) ≈ 6.4 px
This places the line ~6px below the top edge, well inside the waveform body,
no longer coinciding with the TopBar/waveform boundary.

## Dependencies
None
