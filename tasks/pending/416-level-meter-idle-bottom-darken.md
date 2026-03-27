# Task 416: Level Meter Idle Gradient — Darken Bottom to Match Reference (Right Zone RMSE)

## Description
The level meter idle gradient bottom uses `meterSafe.darker(0.3f).withAlpha(0.60f)` as
its lowest colour stop, producing a visible steel-blue at 60% alpha at the bottom of each
meter bar at idle/silence.

Pixel analysis of the Right zone bottom 30% (y=350–500):
- M-LIM average: #535261 (R=83, G=82, B=97) — too bright and too blue
- Reference average: #35313B (R=53, G=49, B=59) — darker and more neutral
- Gap: M-LIM is ~30 units brighter throughout the bottom

The reference meter bottom area at idle is quite dark, matching the dark "empty track" look.
M-LIM's 0.60 alpha blue creates a visibly bright blue-gray bottom.

Wave-22 baseline Right zone RMSE = 23.50%.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` idle gradient block (~line 104, bottom colour stop)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: compiles without error
- [ ] Run: Right zone RMSE (crop 180x500+720+0) → Expected: Right RMSE < 23.50% baseline
- [ ] Run: Full RMSE → Expected: Full ≤ 19.11% (no regression)

## Tests
None

## Technical Details
In `LevelMeter.cpp drawChannel()` idle gradient constructor (~line 102):

**Current:**
```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha(0.10f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker(0.3f).withAlpha(0.60f),  0.0f, barTop2 + barH2,
    false);
```

**Target:** Reduce bottom alpha from 0.60 to ~0.38:
```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha(0.10f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker(0.3f).withAlpha(0.38f),  0.0f, barTop2 + barH2,
    false);
```

Alpha 0.38 reduces blue contribution at bottom by ~37%, bringing average bottom
brightness from ~83 down to approximately ~58 — close to the reference ~53.

Measure the right zone bottom RMSE before and after. The target is to reduce the blue-cast
bottom area without making the active fill look too dim.

Note: the active fill gradient also uses `meterSafe.darker(0.3f)` at the bottom (~line 135).
If reducing idle bottom alpha is insufficient, also adjust the active fill bottom (see task 415).

Right zone RMSE measurement:
```bash
convert /tmp/task-mlim.png -crop 180x500+720+0 +repage /tmp/cur-right.png
convert /tmp/task-ref.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/ref-right.png /tmp/cur-right.png /dev/null 2>&1
```

## Dependencies
None
