# Task 304: GR Meter — Warm Orange/Yellow Color Zones (Pro-L 2 Style)

## Description
The GainReductionMeter currently draws all active GR segments in a single flat red color
(`MLIMColours::gainReduction = 0xffEE3333`). The Pro-L 2 reference uses a warm gradient:
- **Low GR (0–3 dB)**: warm yellow / gold (`#E8C840`)
- **Medium GR (3–9 dB)**: warm orange (`#FF8C00`)
- **High GR (9+ dB)**: red (`#EE3333`)

This zone-based coloring is consistent with how `LevelMeter` already handles its danger/warning/safe
zones. The change applies to `GainReductionMeter::drawBar()`.

The current RMSE for the right panel is 29.60%. The warm color change will make the GR meter look
significantly closer to the reference whenever GR is active.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — `drawBar()`: replace single-colour drawSegments
  call with three zone-based segments, each using a different colour
Modify: `M-LIM/src/ui/Colours.h` — add `grMeterLow` and `grMeterMid` colour constants (yellow and
  orange) if they are not already defined; keep existing `gainReduction` (red) for the top zone
Read: `M-LIM/src/ui/LevelMeter.cpp` — reference for how zone-based drawSegments works

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: Launch standalone on Xvfb :99, play audio or manually invoke `setGainReduction(6.0f)` via test. Confirm GR bar shows yellow at the top (low GR portion), transitioning through orange to red for larger GR values.
- [ ] Code check: `grep -n "grMeterLow\|grMeterMid\|gainReduction" M-LIM/src/ui/GainReductionMeter.cpp` → Expected: output shows the three zone colours referenced in drawBar()

## Tests
None

## Technical Details
In `GainReductionMeter::drawBar()`, replace the existing single-colour segment block:

```cpp
// BEFORE (single colour):
drawSegments (MLIMColours::gainReduction, barTop, fillBot);

// AFTER (three zones):
// Zone boundaries in pixels from top of bar
float zone1Bot = barTop + std::min(fillH, grToFrac(3.0f)  * barH);  // 0–3 dB
float zone2Bot = barTop + std::min(fillH, grToFrac(9.0f)  * barH);  // 3–9 dB
float zone3Bot = barTop + fillH;                                      // 9+ dB

drawSegments (MLIMColours::grMeterLow,    barTop,    zone1Bot);  // warm yellow
drawSegments (MLIMColours::grMeterMid,    zone1Bot,  zone2Bot);  // warm orange
drawSegments (MLIMColours::gainReduction, zone2Bot,  zone3Bot);  // red
```

In `Colours.h`, add near the existing meter colours:
```cpp
const juce::Colour grMeterLow { 0xffE8C840 };  // warm yellow (0–3 dB GR)
const juce::Colour grMeterMid { 0xffFF8C00 };  // warm orange (3–9 dB GR)
// gainReduction (red) is used for 9+ dB GR (already defined)
```

## Dependencies
None
