# Task 289: GainReductionMeter — LED Segment Texture on Empty Bar and Segmented Fill

## Description
The GainReductionMeter currently renders two ways that differ from Pro-L 2:

1. **Empty bar (idle)**: When no GR is applied (`currentGR_ == 0`), `drawBar()` returns early,
   leaving the bar background as a plain dark `displayBackground` rectangle with no texture.
   Pro-L 2's GR meter shows LED segment separator lines across the full bar height even at idle,
   giving it a textured appearance that matches the input meter style.

2. **Filled bar (active)**: When GR is applied, the fill uses a single `fillRect` with a
   gradient. Pro-L 2 uses discrete LED segments (same `kSegH + kSegGap` stride as LevelMeter).

### Required changes in `GainReductionMeter.cpp`:

#### In `drawBar()`:
Replace the current implementation with:
```cpp
void GainReductionMeter::drawBar (juce::Graphics& g,
                                   const juce::Rectangle<float>& barArea) const
{
    // Constants matching LevelMeter segment style
    static constexpr float kSegH   = 3.0f;
    static constexpr float kSegGap = 1.0f;

    // 1. Background track
    g.setColour (MLIMColours::barTrackBackground);
    g.fillRect (barArea);

    // 2. Segment-separator texture across full bar height (LED strip look even at idle)
    g.setColour (MLIMColours::barTrackBackground.brighter (0.35f));
    const float barTop = barArea.getY();
    const float barH   = barArea.getHeight();
    for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
        g.fillRect (barArea.getX(), sy + kSegH, barArea.getWidth(), kSegGap);

    if (currentGR_ <= 0.0f) return;

    // 3. Filled level portion as discrete segments (top-anchored: 0 dB GR = top)
    const float fillH  = grToFrac (currentGR_) * barH;
    const float fillBot = barTop + fillH;

    // GR bar uses a single colour (red) consistent with waveform GR overlay
    auto drawSegments = [&] (juce::Colour colour, float top, float bot)
    {
        if (top >= bot) return;
        g.setColour (colour);
        for (float sy = top; sy < bot; sy += kSegH + kSegGap)
        {
            float segBot = juce::jmin (sy + kSegH, bot);
            if (segBot > sy)
                g.fillRect (barArea.getX(), sy, barArea.getWidth(), segBot - sy);
        }
    };
    drawSegments (MLIMColours::gainReduction, barTop, fillBot);
}
```

No change to `drawPeakTick()`, `drawScale()`, or `drawNumeric()`.

### Visual target:
- Idle (no GR): bar shows subtle horizontal separator lines across full height
- Active (GR applied): filled area shows discrete LED segments from top down
- Segment height/gap: 3 px / 1 px (4 px stride) — matches LevelMeter constants

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — replace `drawBar()` with segmented implementation
Read: `M-LIM/src/ui/GainReductionMeter.h` — class structure, `maxGRdB_`, `currentGR_` fields
Read: `M-LIM/src/ui/Colours.h` — `barTrackBackground`, `gainReduction`, `displayBackground`
Read: `M-LIM/src/ui/LevelMeter.cpp` — reference implementation of segment texture pattern (task 282)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Visual check: at idle (no audio), the GR meter bar shows horizontal separator lines across full height rather than a plain dark rectangle

## Tests
None

## Technical Details
- The `barTrackBackground` constant is `0xff222222`; `.brighter(0.35f)` gives approximately `0xff2E2E2E`
- Use `static constexpr` for `kSegH` and `kSegGap` inside `drawBar()` (or anonymous namespace at top of file)
- The gradient in the old fill code was cosmetic — removing it for a flat colour per segment is fine
- `grToFrac()` is already defined as `juce::jlimit(0.f, 1.f, grDB / maxGRdB_)` — reuse it

## Dependencies
None
