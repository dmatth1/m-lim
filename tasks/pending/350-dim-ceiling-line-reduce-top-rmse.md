# Task 350: Dim waveformCeilingLine — Reduce Top-Area RMSE

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

**Fix (two-part):**

1. **Move the default ceiling position** — change `ceilingDB_` default from -0.1f to **-0.5f**
   in `WaveformDisplay.h`. This shifts the ceiling line ≈6px lower into the waveform body (away
   from the very top edge), so it no longer bleeds into the layout boundary between TopBar and
   waveform.

2. **Reduce ceiling line alpha** — change `waveformCeilingLine` in `Colours.h` from
   `0xCCDD4444` (80% alpha red) to **`0x99CC3333`** (~60% alpha, slightly darker red). This makes
   the line less aggressive when visible; the reference's ceiling indicator is a subtle amber/orange.

These are small changes with no functional impact (the ceiling parameter is still user-adjustable;
only the DEFAULT position changes by 0.4 dBFS).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.h` — change `float ceilingDB_ = -0.1f` to `-0.5f`
Modify: `M-LIM/src/ui/Colours.h` — change `waveformCeilingLine` alpha/color
Read: `M-LIM/src/ui/WaveformDisplay.cpp` — understand drawCeilingLine() to verify position math

## Acceptance Criteria
- [ ] Run: `grep 'ceilingDB_' M-LIM/src/ui/WaveformDisplay.h` → Expected: `-0.5f`
- [ ] Run: `grep 'waveformCeilingLine' M-LIM/src/ui/Colours.h` → Expected: does NOT contain `0xCCDD4444`
- [ ] Run: build succeeds → `cmake --build M-LIM/build -j$(nproc) 2>&1 | tail -1` → Expected: `Built target`
- [ ] Run: launch on Xvfb, screenshot, crop 900x24+0+0 (TopBar strip), sample pixel at x=400,y=21 → Expected: NOT bright red; should be dark (< 0x60 per channel)

## Tests
None

## Technical Details

**WaveformDisplay.h change:**
```cpp
// Line 82 — change default ceiling from -0.1f to -0.5f
float ceilingDB_ = -0.5f;   // output ceiling reference line (dBFS); was -0.1f
```

**Colours.h change:**
```cpp
// was: 0xCCDD4444  (80% alpha, bright red)
const juce::Colour waveformCeilingLine  { 0x99CC3333 };  // ceiling line — muted red, ~60% alpha (task-350)
```

**Why -0.5f:** At the default display range of 30 dBFS (kMaxGRdB=30):
- frac = 0.5/30 = 0.0167
- y_offset = 0.0167 × (waveform_height ≈ 384) ≈ 6.4 px
This places the line ~6px below the top edge, well inside the waveform body,
no longer coinciding with the TopBar/waveform boundary.

## Dependencies
None
