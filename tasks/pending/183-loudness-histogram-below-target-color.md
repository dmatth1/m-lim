# Task 183: LoudnessPanel — Histogram Bars Below Target Should Be Steel Blue Not Near-White

## Description
In `LoudnessPanel::histogramBarColour()`, levels well below the target LUFS threshold are rendered as `MLIMColours::textPrimary.withAlpha(0.75f)` — effectively a near-white/light-gray colour. The reference Pro-L 2 UI clearly shows these bars as a **steel blue** colour matching the level meter safe zone.

Reference confirming the blue colour:
- `/reference-docs/reference-screenshots/prol2-metering.jpg` — the lower half of the histogram (below target) is clearly blue, matching the input/output level meter "safe zone" colour
- `/reference-docs/video-frames/v1-0040.png` — histogram bars in the -20 to -35 LUFS range show blue

Current code (`LoudnessPanel.cpp`, line ~377):
```cpp
else
    return MLIMColours::textPrimary.withAlpha (0.75f);  // near-white — WRONG
```

Should be:
```cpp
else
    return MLIMColours::meterSafe.withAlpha (0.85f);   // steel blue — matches reference
```

`MLIMColours::meterSafe` = `0xff8896AC` (pale steel grey-blue) — exactly the colour used for safe-zone bars on the level meter.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — change `histogramBarColour()` return for the "well below target" case
Read: `M-LIM/src/ui/Colours.h` — confirm `meterSafe` hex value

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, play audio, screenshot — expected: histogram bars below target level are visually steel blue, not white/gray
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (colour-only change; no new test required)

## Technical Details
Change exactly **one line** in `M-LIM/src/ui/LoudnessPanel.cpp` inside `histogramBarColour()`:

```cpp
// BEFORE:
return MLIMColours::textPrimary.withAlpha (0.75f);

// AFTER:
return MLIMColours::meterSafe.withAlpha (0.85f);
```

## Dependencies
None
