# Task 231: Color Palette — Background and Accent Color Parity

## Description
Fine-tune M-LIM's color constants to more closely match FabFilter Pro-L 2's visual appearance. Based on analysis of reference screenshots:

Changes needed in `src/ui/Colours.h`:
1. **Main background**: 0xff1E1E1E → 0xff1A1A1A (slightly darker, matching Pro-L 2's near-black background)
2. **Display background** (waveform area): 0xff141414 → 0xff111118 (dark navy-black, Pro-L 2 has a very slight blue tint in the waveform background)
3. **Widget background**: 0xff2A2A2A → 0xff222230 (slight blue tint for the darker controls area)
4. **Accent blue** (selected buttons, knob arcs): 0xff2196F3 → 0xff2D7EE8 (slightly deeper blue matching Pro-L 2's button highlight)
5. **Control strip top**: 0xff3A3D4A → 0xff353845 (slightly darker, closer to Pro-L 2's knob panel)
6. **Control strip bottom**: 0xff2A2D3A → 0xff252835 (darker bottom)
7. **Knob face**: 0xff505872 → 0xff4A526A (slightly darker and more blue-shifted to match Pro-L 2 knob body)

Also update `LookAndFeel.cpp` to ensure `juce::ResizableWindow::backgroundColourId` is set to the new background color.

After making changes, build to ensure no compile errors.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — update color constants as specified
Modify: `src/ui/LookAndFeel.cpp` — verify background color ID is updated if needed
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — visual reference

## Acceptance Criteria
- [ ] Run: `grep "0xff1A1A1A" /workspace/M-LIM/src/ui/Colours.h` → Expected: 1 match (background color updated)
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0
- [ ] Run: `grep "background.*0xff1" /workspace/M-LIM/src/ui/Colours.h | head -3` → Expected: shows updated dark values

## Tests
None

## Technical Details
- Use `0xffRRGGBB` JUCE color format (alpha=ff for fully opaque)
- Preserve all other color constants that are already well-matched
- Do not change waveform curve colors (inputWaveform, outputWaveform, outputEnvelope, gainReduction) — these are already tuned

## Dependencies
None
