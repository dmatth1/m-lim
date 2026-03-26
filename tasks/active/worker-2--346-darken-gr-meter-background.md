# Task 346: Darken GR Meter Background to Match Reference

## Description

The GainReductionMeter (12px wide strip between waveform and output meter, x=690–702 in
900px image) currently uses the **waveform gradient** (`displayGradientTop / displayGradientBottom`)
as its bar background. This makes the GR meter render at ~(99,99,112) to (80,96,143)
brightness — the same bright blue-gray as the waveform display.

The reference Pro-L 2 at that same screen position shows a very dark background:
~(27–38, 22–32, 26–38) per channel, which is approximately `#241B20` — close to the
output-meter track background (`barTrackBackground = #181818`).

**The per-pixel error in the GR meter strip is ~70 intensity units per channel**, which is
the largest per-pixel difference visible in static (rest-state) elements. Fixing this strip
will directly improve the waveform sub-region RMSE (GR meter falls within x=150–750 waveform
area) and the full-image RMSE.

**Fix**: Change `GainReductionMeter::paint()` and `GainReductionMeter::drawBar()` to use a
dark background instead of the waveform gradient. The segment separators already use
`barTrackBackground.brighter(0.35f)`, so they will remain visible against the dark base.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — change bar background from waveform gradient
Read: `M-LIM/src/ui/Colours.h` — check `barTrackBackground` and `background` constants
Read: `M-LIM/src/ui/GainReductionMeter.h` — understand structure

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-346-after.png" && stop_app` → Expected: screenshot saved
- [ ] Run: pixel sample at GR meter center (x=695, y=200 in plugin crop): should show dark (~20–40 per channel), not bright waveform gradient (~90–130 per channel)

## Tests
None

## Technical Details

In `GainReductionMeter::paint()`, change the background fill from waveform gradient to a
dark solid or near-solid fill:

```cpp
// BEFORE (current) — bright waveform gradient:
juce::ColourGradient grBg = juce::ColourGradient::vertical (
    MLIMColours::displayGradientTop,
    barArea.getY(),
    MLIMColours::displayGradientBottom,
    barArea.getBottom());
g.setGradientFill (grBg);
g.fillRect (barArea);

// AFTER — dark background matching reference (~#241B20):
// Use displayBackground which is very dark (#111118), or barTrackBackground (#181818).
// The reference shows ~(27–38, 22–32, 26–38) at this location.
// barTrackBackground (#181818 = 24,24,24) is close enough.
g.setColour (MLIMColours::barTrackBackground);
g.fillRect (barArea);
```

Do the same replacement inside `GainReductionMeter::drawBar()` where it draws the background
a second time (identical gradient fill in step 1 of drawBar).

The segment separators that use `barTrackBackground.brighter(0.35f)` will still be visible
since they are rendered on top of the new dark background.

Verify visually that:
1. The GR meter strip is now dark (not blue-gray waveform gradient)
2. The segment separator lines are still visible as subtle horizontal marks
3. The GR bar fills (yellow/orange/red for active gain reduction) still render correctly

## Dependencies
None (can run in parallel with other tasks)
