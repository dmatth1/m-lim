# Task 457: LUFS Readout Large Number — Increase Size and Match Pro-L 2 Positioning

## Description
In Pro-L 2 (prol2-main-ui.jpg, v1-0008.png, v1-0020.png), the large LUFS readout number (e.g., "-13.2", "-20.9") is prominently displayed in the lower-right area of the plugin, right-aligned with large bold text (~38-48pt). It uses a warm orange color for normal readings.

M-LIM's LUFS readout in the loudness panel shows "---" (idle) in the lower section, with "LUFS" label below. The layout approximately matches but the number needs to be:
1. Larger and bolder for visual impact
2. Right-aligned within the panel (Pro-L 2 right-aligns the number)
3. The "LUFS" label should be positioned to the right of/below the number

Current implementation in `LoudnessPanel.cpp` uses 38pt bold font for the readout, which is close but the overall readout area may need better positioning to match Pro-L 2's right-aligned large number aesthetic.

**Fix approach**: Verify the LUFS readout is right-justified within its area. Ensure the "LUFS" suffix label appears to the right of the number, not below it, matching Pro-L 2 layout. Consider increasing font to 42pt if space permits.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LoudnessPanel.cpp` — adjust LUFS readout section: right-align the number, position "LUFS" label to the right
Read: `src/ui/LoudnessPanel.h` — layout constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
The large readout area is 48px tall in the current implementation. In Pro-L 2, the number is right-aligned within the panel with "LUFS" appearing as a small label to the right at the top. The number itself uses `lufsReadoutGood` (0xffE87828 warm orange) which is correct.

## Dependencies
None
