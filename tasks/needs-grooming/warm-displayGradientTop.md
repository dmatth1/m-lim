# Task: Warm displayGradientTop slightly to reduce blue cast at top of waveform

## Description
Pixel analysis (wave22 audit) shows the top waveform zone (y=10-110px of 500px) averages
M-LIM (47,46,53) vs Reference (52,44,47). M-LIM is 5 units too dark in red, +2 in green,
+6 too blue. The waveform top needs slightly warmer tone (more red, less blue).

Current: 0xff28242A (R:40, G:36, B:42)
Needed shift: +12R, +8G, +5B based on gap analysis and alpha-blend back-calculation
Recommended: 0xff342C2F (R:52, G:44, B:47) — which would give close to reference top-zone color

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `displayGradientTop` constant

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: crop 650x100+0+10 from M-LIM 900x500 crop → Expected: avg R closer to 52 (currently 47), B closer to 47 (currently 53)
- [ ] Run: full RMSE → Expected: Wave RMSE does not worsen vs 19.31% baseline

## Tests
None

## Technical Details
- Baseline (wave 22): Full=19.46%, Wave=19.31%
- The top zone measurement: crop 650x100+0+10 from the 900x500 M-LIM crop
- Reference top avg: R=52, G=44, B=47 — warmer/redder, less blue than current
- This is a small change — verify it doesn't negatively impact mid-zone match which is currently close

## Dependencies
None
