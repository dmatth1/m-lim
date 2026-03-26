# Task 297: Level Meters — Darken Inactive Segment Background

## Description
The level meter inactive segment background uses `barTrackBackground = #222222`. The segment gap
lines are drawn with `barTrackBackground.brighter(0.35f)` which produces approximately `#2F2F2F` —
these gap lines are visible as subtle horizontal stripes across the meter bar.

Looking at the M-LIM left meter strip vs the reference, the M-LIM meter has too-bright gap lines
creating a visible striped texture. In the reference, the inactive meter appears as a nearly solid
dark area with very subtle segmentation.

**Fix**: Remove the segment gap lines from the INACTIVE portion of the meter bars — draw them only
in the filled (active) segment region. In the empty (inactive) region, just show the `barTrackBackground`.
This makes the empty meter blend more smoothly with the dark plugin background.

Additionally, reduce `barTrackBackground` in Colours.h from `#222222` to `#181818` to match the
reference's very dark meter background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — in `drawChannel()`, move the segment texture lines from
        "across full bar height" to only inside the filled level portion
Modify: `M-LIM/src/ui/Colours.h` — darken `barTrackBackground` from `#222222` to `#181818`

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc)` → Expected: exits 0
- [ ] Visual: Input and output meter bars show a near-solid dark background when empty (no audio),
      with no visible segment stripe pattern in the empty region
- [ ] Visual: When audio is active (if testable), the filled portion retains segmented appearance
- [ ] RMSE check on left meter region: improvement vs current 33.36% (or vs post-task-293 baseline)

## Tests
None

## Technical Details
In `LevelMeter::drawChannel()`, the current code draws segment gap lines across the FULL bar height:
```cpp
// Segment-line texture across full bar height (filled + empty)
g.setColour (MLIMColours::barTrackBackground.brighter (0.35f));
for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
    g.fillRect (bar.getX(), sy + kSegH, bar.getWidth(), kSegGap);
```

Change to: draw gap lines only in the FILLED region (below `fillTop`):
```cpp
// Segment-line texture only in the filled (active level) region
if (fillH > 0.0f)
{
    g.setColour (MLIMColours::barTrackBackground.brighter (0.35f));
    for (float sy = fillTop; sy < barTop + barH; sy += kSegH + kSegGap)
        g.fillRect (bar.getX(), sy + kSegH, bar.getWidth(), kSegGap);
}
```

Also in `Colours.h`:
```cpp
const juce::Colour barTrackBackground { 0xff181818 };  // was 0xff222222, darker to match reference
```

## Dependencies
None
