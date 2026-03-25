# Task 063: Waveform Display Gradient Background

## Description
Pro-L 2's waveform display area has a subtle blue/gray gradient background rather than flat black. The background is darkest at the edges and has a slight blue-lavender wash that gets marginally lighter toward the center, creating depth. Task 022 specifies only "dark background with subtle horizontal grid lines" — missing the gradient character. This is a subtle but noticeable visual detail for UI parity.

Reference: See `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (waveform area shows subtle gradient), `/reference-docs/video-frames/v1-0042.png` (full-width waveform showing gradient clearly), `/reference-docs/video-frames/v2-0020.png`.

## Produces
None

## Consumes
WaveformDisplayInterface
ColoursDefinition

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — replace flat background fill with gradient
Read: `M-LIM/src/ui/Colours.h` — color constants (may need new gradient colors)
Modify: `M-LIM/src/ui/Colours.h` — add displayGradientTop and displayGradientBottom colors if needed

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "gradient\|Gradient\|ColourGradient" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: at least 1 match

## Tests
None (visual detail)

## Technical Details
- Replace the flat `displayBackground` fill in WaveformDisplay::paint() with a `juce::ColourGradient`
- Gradient direction: top to bottom (vertical)
- Top color: very dark (#0D0D12 or similar, near-black with slight blue tint)
- Bottom color: slightly lighter dark blue-gray (#1A1A2E or similar)
- The gradient should be VERY subtle — the background is still predominantly dark, just with a slight cool-toned wash
- Grid lines should be drawn ON TOP of the gradient
- Use `juce::ColourGradient::vertical()` for efficiency
- Consider adding `displayGradientTop` and `displayGradientBottom` to the MLIMColours namespace for consistency

## Dependencies
Requires task 022
