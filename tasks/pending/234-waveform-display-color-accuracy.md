# Task 234: Waveform Display — Color and Rendering Accuracy

## Description
Fine-tune the waveform display rendering to better match FabFilter Pro-L 2's visual style. Based on reference screenshots:

1. **Background gradient**: Pro-L 2's waveform background is very dark navy-blue. The current gradient colors are close but need verification. Adjust `displayGradientTop` to `0xff0E1020` (darker) and `displayGradientBottom` to `0xff161C30` (medium dark navy).

2. **Input waveform bars**: Pro-L 2 shows dark blue/charcoal columns for the waveform. Current `inputWaveform = 0xA8607898` (blue-purple). Adjust to `0xA84A5E80` (slightly darker, cooler blue).

3. **Output envelope**: The golden/amber output envelope line should be clearly visible. Current `outputEnvelope = 0xCCE8C878`. This is close — keep it but ensure the line rendering uses a stroke of 1.5-2.0px for clear visibility.

4. **GR display**: The gain reduction bars should show as dark red/maroon fills, not just red lines. Verify `gainReduction = 0xffFF4444` is used at reduced alpha for the fill area (try 0x60FF4444 for fill, full red for the peak).

5. **dB scale**: The right-edge dB scale text in Pro-L 2 is small, light gray. Ensure WaveformDisplay draws these labels clearly at kFontSizeSmall.

6. **Grid lines**: Horizontal grid lines at dB intervals should use `waveformGridLine = 0xff2E3040`. Verify they're being drawn with this color.

Read the WaveformDisplay paint() method fully before making changes. Only adjust colors and stroke widths — do not change the layout logic.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — adjust displayGradientTop, displayGradientBottom, inputWaveform colors
Read: `src/ui/WaveformDisplay.cpp` — understand full paint() method before changing
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — primary reference
Read: `/reference-docs/video-frames/v1-0005.png` — close-up of waveform rendering

## Acceptance Criteria
- [ ] Run: `grep "displayGradientTop" /workspace/M-LIM/src/ui/Colours.h` → Expected: shows new darker value 0xff0E1020
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0

## Tests
None

## Technical Details
- Only change color constants in Colours.h, not rendering logic
- The output envelope line stroke should be at least 1.5px to be visible in screenshots
- Verify WaveformDisplay.cpp renders the output level as a line/envelope, not just bars

## Dependencies
None
