# Task 347: LevelMeter — Add LED Segment Separators at Idle (Rest State)

## Description

The `GainReductionMeter` already draws LED-style segment separator lines across the full
bar height even at idle (no gain reduction), creating a subtle segmented/LED grid texture
visible at rest. The `LevelMeter` (output level meter, 58px wide) does NOT have this feature
— it shows a solid flat `barTrackBackground` with no texture.

The reference Pro-L 2 output meter shows a distinctive LED segment appearance with thin
horizontal gap lines between segments even when the bars are fully unlit (rest state).

**Fix**: Add the same segment separator loop to `LevelMeter::drawChannel()` background
rendering that `GainReductionMeter::drawBar()` already uses.

Reference implementation from `GainReductionMeter::drawBar()`:
```cpp
// Segment-separator texture across full bar height (LED strip look even at idle)
g.setColour (MLIMColours::barTrackBackground.brighter (0.35f));
const float barTop = barArea.getY();
const float barH   = barArea.getHeight();
for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
    g.fillRect (barArea.getX(), sy + kSegH, barArea.getWidth(), kSegGap);
```

Apply equivalent segment separator rendering to `LevelMeter::drawChannel()` after drawing
the background track. Use the same or similar segment constants (kSegH=3px, kSegGap=1px).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — add segment separator to `drawChannel()` background
Read: `M-LIM/src/ui/GainReductionMeter.cpp` — reference implementation of segment separators

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: visual inspection of output meter at rest — should show subtle horizontal LED-style segment lines across the dark bar background
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-347-after.png" && stop_app` → Expected: screenshot saved, segment lines visible in output meter at rest

## Tests
None

## Technical Details

In `LevelMeter::drawChannel()`, after drawing the background track, add segment separators:

```cpp
void LevelMeter::drawChannel (juce::Graphics& g,
                               juce::Rectangle<float> bar,
                               float levelDB,
                               float peakDB,
                               bool  clipped) const
{
    const float barH   = bar.getHeight();
    const float barTop = bar.getY();

    // Background track
    g.setColour (MLIMColours::barTrackBackground);
    g.fillRect (bar);

    // ADD: LED segment separator lines (matches GainReductionMeter pattern)
    static constexpr float kSegH   = 3.0f;
    static constexpr float kSegGap = 1.0f;
    g.setColour (MLIMColours::barTrackBackground.brighter (0.25f));
    for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
        g.fillRect (bar.getX(), sy + kSegH, bar.getWidth(), kSegGap);

    // ... rest of existing code unchanged ...
```

Note: Use `brighter(0.25f)` (slightly less bright than GainReductionMeter's 0.35f) for
a subtler effect appropriate to the output meter. Keep the same segment geometry (3px
segment + 1px gap).

The filled level bars and peak hold line rendering remains unchanged — the separators
only affect the dark background portion (idle appearance).

## Dependencies
None (can run in parallel with task 346)
