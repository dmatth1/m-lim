# Task 266: LevelMeter — Segment-Line Texture on Empty Track Background

## Description
The current LevelMeter draws segmented LED strips only for the **filled** (signal level) portion
of the bar. The empty track background above the filled area is a solid `barTrackBackground`
colour with no segment lines. This means at idle (no audio) the meters look like plain dark
rectangles — identical to a pre-segment meter.

The FabFilter Pro-L 2 reference shows segment separator lines running across the **full height**
of each bar, visible even in the empty portion, giving a textured "dark LED strip" look at all
signal levels including idle.

### Visual target:
- Fine horizontal separator lines (1 px, dark/track colour) drawn at regular intervals across
  the **entire bar height** (both filled and empty portions)
- Same `kSegH + kSegGap` stride as the filled segments (4 px total)
- Lines colour: slightly lighter than barTrackBackground to create visible segmentation without
  being distracting (e.g. `MLIMColours::barTrackBackground.brighter(0.4f)`)

### Per-region RMSE (VisualParityAuditor 2026-03-26):
- Left level meters: 29.6% RMSE; right panel: 24.5% RMSE
- Adding segment texture to the empty track improves the idle-state visual match

### Required Change in `LevelMeter.cpp` → `drawChannel()`:

After the background track fill (line ~79), add a segment-separator overlay across the full bar:

```cpp
// Segment-line texture across full bar height (filled + empty)
g.setColour(MLIMColours::barTrackBackground.brighter(0.35f));
for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
    g.fillRect(bar.getX(), sy + kSegH, bar.getWidth(), kSegGap);
```

This draws the separator lines BEFORE the filled segments, so the filled zones paint over their
share and the separator lines only remain visible in the empty portion above the fill.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — add segment-line texture loop after background track fill in `drawChannel()`
Read: `src/ui/Colours.h` — `barTrackBackground` colour constant

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — level meter bars show horizontal segment separator lines across the full bar height even at idle (no audio), not just in the filled zone

## Tests
None

## Technical Details
- Insert the new loop at approximately line 80 of `LevelMeter.cpp`, immediately after the
  `g.fillRect(bar)` background fill and before the `dbToNorm(levelDB)` fill calculation
- The separator colour should be dark but visible: `barTrackBackground.brighter(0.35f)` gives
  approximately `0xff2E2E2E` (dark charcoal, visible against `0xff222222` background)
- The `kSegH` and `kSegGap` constants are already defined in the anonymous namespace at the top
  of the file — reuse them directly
- The filled segment zones drawn afterward will paint over their portion of the separators,
  maintaining visual continuity between empty and filled sections

## Dependencies
None
