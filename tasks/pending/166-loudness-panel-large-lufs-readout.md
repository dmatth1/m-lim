# Task 166: LoudnessPanel — Add Large-Format LUFS Numeric Readout

## Description
The reference Pro-L 2 loudness panel shows a prominent large-format LUFS number (e.g. "-13.2") at the bottom of the panel, accompanied by a "LUFS" unit label and a mode selector button ("Short Term", "Momentary", or "Integrated"). This is the most visually dominant element of the loudness panel — the user's primary at-a-glance reading.

The current `LoudnessPanel` implementation only renders small text rows (Momentary, Short-Term, Integrated, Range, True Peak) and entirely lacks this large display. Screenshots taken of the running plugin confirm this.

Reference frames showing the large readout clearly:
- `/reference-docs/video-frames/v1-0015.png` → large "-9.4 LUFS" visible at bottom-right
- `/reference-docs/video-frames/v1-0035.png` → large "-7.6 LUFS" visible with "Short Term" label
- `/reference-docs/reference-screenshots/prol2-main-ui.jpg` → "-13.2 LUFS" with "Short Term" selector

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.h` — add large readout layout constants and state
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — add `drawLargeReadout()` method and update `resized()` / `paint()`
Read: `M-LIM/src/ui/Colours.h` — use existing colour constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, launch on Xvfb, screenshot — expected: loudness panel shows a large LUFS number (placeholder "---" when no audio) in font size ~28-32pt, bold, centred in its display area
- [ ] Run: `cd build && ctest --output-on-failure -R LoudnessPanel` → Expected: all existing tests pass

## Tests
None (display-only change; existing tests must continue to pass)

## Technical Details

**Layout change:**
Add a "large readout" strip at the very bottom of `LoudnessPanel`, approximately 48 px tall. This strip replaces or supplements the current small readout rows. The existing rows (Momentary, Short-Term, Integrated, Range, True Peak) can stay, but reduce their vertical area so the large readout fits.

**Large readout content:**
- Show the value of whichever measurement mode is active: Short-Term by default
- The `measurementMode_` (already exists as `ControlStrip::MeasurementMode`) is separate from `LoudnessPanel`, so add a `setLargeReadoutMode(enum)` or simply always show Short-Term LUFS in the large display (matching the reference default)
- Display the active LUFS value in bold ~28pt font; colour = `textPrimary` when below target, `meterDanger` when above target
- Show "LUFS" in ~10pt font below the number (or to the right)

**Suggested layout constants to add to LoudnessPanel.h:**
```cpp
static constexpr int kLargeReadoutH = 48;  // height of large-number strip at bottom
```

**Font:**
```cpp
g.setFont(juce::Font(28.0f, juce::Font::bold));
```

**Colour logic:**
```cpp
juce::Colour valColour = (shortTerm_ < targetLUFS_)
    ? MLIMColours::textPrimary : MLIMColours::meterDanger;
```

## Dependencies
None
