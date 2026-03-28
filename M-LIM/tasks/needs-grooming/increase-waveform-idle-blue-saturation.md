# Task: Increase Waveform Idle Mid-Zone Blue Saturation

## Description
The waveform display's mid-zone idle appearance lacks blue saturation compared to Pro-L 2 reference. Pixel comparison at waveform center (y~170 in editor, ~35% down from top): M-LIM renders ~(55,54,66) while reference shows ~(105,118,160). The reference has much stronger blue saturation (B=160 vs B=66, delta 94 units) and overall brightness (avg=128 vs avg=58). Existing grooming tasks address the lower zone brightness and upper zone darkness, but the mid-zone blue saturation gap is the single largest per-channel discrepancy in the waveform.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — idle fill layers (lines ~295-393), particularly the mid-zone tent fill using `waveformIdleMidFill` at alpha 0.52; increase alpha and/or add additional blue-tinted idle fill in the 25%-60% height range
Modify: `src/ui/Colours.h` — `waveformIdleMidFill` (0xff828AA5 = 130,138,165) may need higher blue channel; `inputWaveform` alpha levels in the idle simulation may need increasing
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference waveform mid-zone at rest with audio

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at waveform mid-zone → Expected: B channel increases from ~66 toward 120+; overall brightness increases

## Tests
None

## Technical Details
- The idle waveform composite at mid-zone is dominated by the displayGradientTop→Bottom gradient plus the mid-zone tent fill (waveformIdleMidFill at peak alpha 0.52)
- The mid-zone fill at lines ~315-330 creates a triangle pattern from 36% to 82% height with peak at center
- At y=35% (the problematic area), this tent fill is near zero alpha, so only the dark gradient shows through
- Fix approach: extend the mid-zone fill to start higher (e.g., from 20% instead of 36%), or add a new idle fill layer covering 20%-50% with blue-tinted color at alpha 0.4-0.6
- The reference mid-zone color (105,118,160) has a strong blue component — the fill color should emphasize blue
- Be careful: the upper zone (top 15%) should stay dark per the reference; only the 20%-50% band needs brightening

## Dependencies
None
