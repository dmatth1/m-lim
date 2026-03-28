# Task: Right Panel dB Scale Range Mismatch — Should Show +12 to -32 dB

## Description
The loudness panel dB scale in M-LIM shows 0 to -35 LUFS range, but the Pro-L 2 reference (video frame v1-0002) shows a scale from approximately +12 dB to -32 dB on the right panel histogram. The reference's histogram area has labeled dB markings (+12, +4, -4, -8, -12, -16, -20, -24, -28, -32) that span a wider range than our current -35 to 0 LUFS. The right panel dB scale labels and histogram range should be verified and adjusted to match the reference's range.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/ui/LoudnessPanel.cpp` — histogram scale strip rendering and bin range
Read: `src/ui/LoudnessPanel.h` — histogram bin configuration
Read: `/reference-docs/video-frames/v1-0002.png` — clear view of reference right panel dB scale
Read: `/reference-docs/video-frames/v1-0010.png` — another reference showing histogram scale

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot of right panel → Expected: dB scale labels match reference range

## Tests
None

## Technical Details
- Current: 70 bins covering -35 to 0 LUFS in 0.5 dB steps, with a 26px scale strip showing labels
- Reference: shows labels at +12, +4, -4, -8, -12, -16, -20, -24, -28, -32 dB — this is a ~44 dB range
- The reference right panel appears to show the loudness distribution relative to the target level, not absolute LUFS
- The reference scale may use "dB relative to target" rather than absolute LUFS
- Verify whether the histogram bin range needs expanding, or if just the scale labels need adjustment
- This contributes to the 26.3% right panel RMSE

## Dependencies
None
