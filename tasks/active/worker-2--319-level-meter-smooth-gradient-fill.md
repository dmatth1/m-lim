# Task 319: Level Meter Smooth Gradient Fill (Remove LED Segments)

## Description
The output level meter in M-LIM uses a segmented LED-strip rendering (`kSegH=3, kSegGap=1`
in LevelMeter.cpp). Pro-L 2 uses a **continuous gradient fill** — no visible segment gaps.

Replace the segmented approach with a solid continuous fill. Also improve the color zones:
- Current: three discrete flat-colour zones (safe=muted-blue, warn=yellow, danger=red)
- Target: smooth gradient that transitions from deeper blue at minimum, brightening as it
  approaches the yellow zone, then yellow, then orange/red

This change improves visual parity when audio is playing. At idle (empty bars) the effect is
minimal, but during playback the meters will look like Pro-L 2's smooth gradient meters.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — replace segmented rendering with solid fill + gradient zones
Read: `src/ui/Colours.h` — current meter color constants (`meterSafe`, `meterWarning`, `meterDanger`)

## Acceptance Criteria
- [ ] Run: build the standalone → Expected: compiles without errors
- [ ] Run: launch and visually inspect — level meter bars should show solid fill (no LED gaps visible when a bar is filled)
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`

## Tests
None

## Technical Details
In `LevelMeter.cpp`, the `drawChannel()` function currently draws segments like:
```cpp
auto drawSegments = [&] (juce::Colour colour, float top, float bot) {
    ...
    for (float sy = top; sy < bot; sy += kSegH + kSegGap) { ... }
};
```

Replace each `drawSegments(colour, top, bot)` call with a direct solid fill:
```cpp
auto drawSolid = [&] (juce::Colour colour, float top, float bot) {
    if (top >= bot) return;
    g.setColour(colour);
    g.fillRect(bar.getX(), top, bar.getWidth(), bot - top);
};
```

Also remove the segment-line texture pass (the loop that draws gap lines into the filled region).

The segment-line texture loop that draws into the filled region (lines ~134–140 in LevelMeter.cpp):
```cpp
if (fillH > 0.0f) {
    g.setColour(MLIMColours::barTrackBackground.brighter(0.35f));
    for (float sy = fillTop; sy < ...) g.fillRect(...);
}
```
Remove this block entirely.

The result should be solid colour blocks per zone (safe/warn/danger) with no LED gaps.

## Dependencies
None
