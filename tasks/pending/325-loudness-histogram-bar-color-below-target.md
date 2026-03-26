# Task 325: Loudness Histogram Bar Color — Blue-Gray for Below-Target Bins

## Description
The loudness histogram in `LoudnessPanel` currently uses golden/warm colors (`lufsReadoutGood` = #E8C040)
for bins below the target level. The Pro-L 2 reference shows pale steel-blue for below-target bins,
with warm/golden only for at-or-above-target bins.

Observed in reference (prol2-main-ui.jpg, prol2-goodies.jpg, v1-0012.png, v1-0014.png):
- Bins ABOVE target (diff ≥ 0): warm golden/yellow
- Bins NEAR target (−2 ≤ diff < 0): warm orange/golden
- Bins BELOW target (diff < −2): **pale steel-blue** (similar to `meterSafe` color)

Current implementation in `histogramBarColour()`:
- `diff >= -6.0f` → `lufsReadoutGood`  (warm golden) ← WRONG
- else → `lufsReadoutGood.withAlpha(0.65f)` (dimmer golden) ← WRONG

Fix: replace below-target colors with muted steel-blue.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — `histogramBarColour()` function, lines 403–416
Read:   `M-LIM/src/ui/Colours.h` — `meterSafe`, `meterAtTarget`, `lufsReadoutGood` color constants

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc)` → Expected: build succeeds, 0 errors
- [ ] Run: launch standalone, take screenshot → Expected: histogram bars at below-target LUFS bins show muted steel-blue (similar to `meterSafe = 0xff6879A0`) not warm golden

## Tests
None

## Technical Details
Change `histogramBarColour()` logic:
```cpp
juce::Colour LoudnessPanel::histogramBarColour(float binLUFS, float targetLUFS) noexcept
{
    const float diff = binLUFS - targetLUFS;
    if (diff >= 0.0f)
        return MLIMColours::meterWarning;                              // above target: yellow
    else if (diff >= -2.0f)
        return MLIMColours::meterAtTarget;                             // at target ±2: orange
    else if (diff >= -6.0f)
        return MLIMColours::meterSafe.withAlpha(0.85f);               // approaching: steel-blue
    else
        return MLIMColours::meterSafe.withAlpha(0.55f);               // well below: dimmer steel-blue
}
```
The `meterSafe` color is `0xff6879A0` — a muted steel-blue matching the Pro-L 2 below-target histogram appearance.

## Dependencies
None
