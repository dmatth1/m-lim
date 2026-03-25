# Task 135: Waveform Peak Labels Should Show Input dBFS Values, Not GR Amount

## Description
In `WaveformDisplay::drawPeakMarkers()`, peak labels are drawn showing the **gain reduction amount** (e.g., "3.5dB", "6.2dB"). However, in the Pro-L 2 reference (`prol2-main-ui.jpg`, `v1-0030.png`), the peak labels in the waveform display show the **input peak level in dBFS** (e.g., "-0.5 dB", "-1.6 dB", "-5.0 dB") — these are the input signal peaks that triggered limiting.

The labels are positioned at the **top of the gain reduction zone** (at the input peak position, not below the GR depth). In the reference:
- Labels appear as yellow/gold rounded-rectangle badges with dark text
- They show the INPUT level that was limited (e.g., "-0.5 dB" means the input hit -0.5 dBFS)
- They are positioned near the top of the waveform where the gain reduction begins
- Format: `"-0.5"` (no "dB" suffix in reference, just the numeric value in a badge)

**Current implementation issues in `drawPeakMarkers()`:**
1. Uses `f.gainReduction` to find peaks — this finds where GR is highest, not where input peaks are
2. Labels show `grDB + "dB"` — should show the input peak level in dBFS
3. Labels are positioned at `area.getY() + grToHeight(...)` — this places them at the GR depth, not at the input peak position

**Fix:**
1. Detect peaks in the **input waveform** (using `f.inputLevel`), not GR
2. Convert the peak input level to dBFS: `dBFS = 20 * log10(inputLevel)` (or use the existing `levelToY()` logic)
3. Position the label at the `y = levelToY(f.inputLevel, area)` position (top of the waveform fill at that point)
4. Show the label text as just the dBFS value, e.g., `"-0.5"` with no "dB" suffix, in gold color with dark background badge

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — rewrite `drawPeakMarkers()` to detect input level peaks and display dBFS values
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — peak labels showing "-0.5 dB", "-1.6 dB"
Read: `/reference-docs/video-frames/v1-0030.png` — shows "-5.0 dB", "-5.6 dB", "-4.5 dB" peak labels

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None

## Technical Details
Replace the current `drawPeakMarkers()` body with logic that:

1. Builds a vector of input levels: `std::vector<float> inp(total)` filled with `f.inputLevel`
2. Finds local maxima in `inp` above a threshold (e.g., linear 0.5 = ~-6 dBFS)
3. For each local maximum:
   - Compute `y = levelToY(inp[i], area)` — places badge at the top of the input waveform fill
   - Compute `dBFS = 20.0f * std::log10(juce::jlimit(1e-6f, 1.0f, inp[i]))`
   - Format label: `juce::String(dBFS, 1)` (negative values like "-0.5", "-3.2")
   - Draw gold rounded-rectangle badge with dark text, positioned slightly ABOVE y (label sits above the peak)
4. The badge should be positioned so it doesn't overlap with other badges — skip drawing if two peaks are within ~15 pixels of each other horizontally

## Dependencies
None
