# Task 098: Waveform Display Fill Colours Must Be Dark Navy Not Cyan

## Description
Task 022 (WaveformDisplay) specifies the input waveform fill as "light cyan, semi-transparent" and uses `MLIMColours::inputWaveform` which is currently defined as `0x994FC3F7` (60%-opaque bright cyan). This is **wrong**.

Pixel analysis of the Pro-L 2 reference and visual inspection of multiple video frames show the input waveform body fill is a **very dark navy/charcoal blue** — nearly matching the dark background, with peaks appearing slightly lighter. The bright red gain reduction area (from the top of the display downward) is the visually dominant element, not the waveform fill.

Reference observations from `/reference-docs/video-frames/v1-0040.png` (heavy limiting applied):
- Waveform background: `#141418` (near-black with faint blue)
- Waveform body fill: dark, essentially indistinguishable from background except at peaks
- Waveform peaks: slightly lighter, `#2A2830` range
- Gain reduction area: bright red `#CC2030` from top downward
- Output waveform envelope line: faint gold/tan `#C8A069` colour

This task corrects `Colours.h` and ensures task 022 implementors use the correct paint spec.

Reference: `/reference-docs/reference-screenshots/prol2-main-ui.jpg`, `/reference-docs/video-frames/v1-0020.png`, `/reference-docs/video-frames/v1-0040.png`.

## Produces
None

## Consumes
ColoursDefinition
WaveformDisplayInterface

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — fix `inputWaveform` and `outputWaveform` colour constants
Read: `M-LIM/src/ui/WaveformDisplay.cpp` — update if already implemented; if not yet, this informs task 022

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "inputWaveform" M-LIM/src/ui/Colours.h` → Expected: value does NOT contain `4FC3F7` (not bright cyan)
- [ ] Run: `grep "outputWaveform" M-LIM/src/ui/Colours.h` → Expected: value is present and dark-toned

## Tests
None (visual colour constant)

## Technical Details

**Fix the colour constants in `Colours.h`:**

Pro-L 2 waveform visual layers (back to front):
1. Background: `displayBackground` (`#141414`) — already correct
2. **Input waveform BODY fill**: Very dark navy blue, semi-transparent
   - `inputWaveform { 0x70202840 }` — ~44% alpha, very dark navy (was `0x994FC3F7`)
3. **Input waveform PEAKS**: Slightly lighter at the peak tips (can be achieved by rendering the peak line separately in a lighter colour or by making the fill slightly brighter at higher amplitude values)
4. **Output waveform envelope line**: Faint gold/amber line showing the limited output level
   - Add new constant: `outputEnvelope { 0x80B89040 }` — semi-transparent amber/tan
5. **Gain reduction fill**: Bright red from top of display downward
   - `gainReduction { 0xffFF4444 }` — already correct

The `outputWaveform` constant `0x801565C0` (50% dark blue) is closer to correct than `inputWaveform` but should also be updated:
- `outputWaveform { 0x60182848 }` — slightly darker navy for the output fill

**Paint order spec for task 022 implementors:**
```
1. Fill background with displayBackground
2. Draw horizontal dB grid lines (subtle, panelBorder colour)
3. Fill INPUT waveform area with inputWaveform (dark navy fill from 0 dB line DOWN to input peak)
4. Fill OUTPUT waveform area with outputWaveform (slightly lighter, from 0 dB DOWN to output peak — BELOW the input fill)
5. Fill GAIN REDUCTION from display TOP downward to the output ceiling line, gainReduction colour
6. Draw OUTPUT ENVELOPE line (single-pixel or 2px line at the output waveform edge) in outputEnvelope
7. Draw ceiling reference line in a subtle white
8. Draw peak labels (yellow boxes with dB values)
9. Draw dB scale on left edge
```

## Dependencies
Requires task 003 (Colours.h exists)
Note: Must be completed BEFORE or together with task 022 (WaveformDisplay) to ensure correct colours
