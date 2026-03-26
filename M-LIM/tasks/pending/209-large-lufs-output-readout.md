# Task 209: Loudness Panel — Large Prominent LUFS Numeric Readout

## Description
The FabFilter Pro-L 2 reference shows a **very large numeric LUFS readout** (e.g. "-18.3") in the lower-right area of the plugin, with a "Short Term" label beneath it and a "LUFS" unit label. This is the dominant loudness display element.

### Reference appearance (video frames v1-0003, v1-0004, prol2-main-ui.jpg):
- Large bold number, e.g. "-18.3", approximately 28–36px font height
- Located at the bottom of the loudness panel / right side of the plugin
- "LUFS" label in smaller text to the right or below
- "Short Term" label below in secondary grey text
- The readout is the most visually prominent element in the loudness panel
- Colour: warm golden-yellow (MLIMColours::lufsReadoutGood) when below target, or red/yellow when over

### Current state:
LoudnessPanel shows Momentary/Short-Term/Integrated/Range/True Peak as separate rows with small (10–11px) values — none are prominently large.

### Required Change:
In `LoudnessPanel.cpp` → `paint()` / layout:
1. Add a large numeric display section at the BOTTOM of the loudness panel (below the histogram or LUFS rows)
2. Display the Short Term LUFS value in large font (~30px bold)
3. Add "LUFS" in ~10px text to the right
4. Add "Short Term" label below in MLIMColours::textSecondary
5. Colour the large number with `histogramBarColour(shortTerm_, targetLUFS_)` logic

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LoudnessPanel.cpp` — add large readout section in paint(), update resized() to reserve space
Modify: `src/ui/LoudnessPanel.h` — add kLargeReadoutH constant if needed
Read: `src/ui/Colours.h` — lufsReadoutGood, meterDanger, textSecondary
Read: `/reference-docs/video-frames/v1-0003.png` — reference showing "-18.3 LUFS Short Term" display

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — bottom of loudness panel shows a large (28–36px) bold LUFS numeric readout with "Short Term" label below it

## Tests
None

## Technical Details
In `LoudnessPanel::paint()`, add after existing rows:
```cpp
// Large readout section (bottom ~60px of panel)
auto readoutArea = getLocalBounds().removeFromBottom(60).toFloat();
juce::String val = (shortTerm_ <= -99.0f) ? "---"
                 : juce::String(shortTerm_, 1);
g.setFont(juce::Font(30.0f, juce::Font::bold));
g.setColour(histogramBarColour(shortTerm_, targetLUFS_));
g.drawFittedText(val, readoutArea.removeFromTop(38).toNearestInt(),
                 juce::Justification::centredRight, 1);
// "Short Term" label
g.setFont(juce::Font(9.0f));
g.setColour(MLIMColours::textSecondary);
g.drawFittedText("Short Term", readoutArea.toNearestInt(),
                 juce::Justification::centredRight, 1);
```
- Reserve the bottom 60px in `resized()` so existing rows don't overlap

## Dependencies
None
