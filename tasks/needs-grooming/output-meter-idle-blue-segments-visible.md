# Task: Output Meter Idle State — Show Visible Blue Segments in Lower Zone

## Description
The output meter (right side, 100px wide) is the highest RMSE subregion at 31.6%. In idle state (no audio), the Pro-L 2 reference shows the meter bars with visible blue-ish segments in the lower portion (below -18 dBFS), creating a subtle "lit from below" appearance even at silence. M-LIM's idle meter is almost entirely dark with only the faintest gradient — the bottom segments should be noticeably brighter.

In the reference screenshots (prol2-main-ui.jpg, prol2-intro.jpg), the output meter bars show:
- Top: nearly black (danger zone)
- Middle: very dark
- Bottom 30%: visibly blue-tinted segments (meterSafe color at ~0.25-0.40 alpha)

Current idle gradient in `LevelMeter.cpp` lines 102-121 has `meterSafe.withAlpha(0.80f)` at the very bottom but ramps too gradually — the visible blue only kicks in at the last ~10% of bar height. The idle fill should be more prominent in the lower 40% to match the reference.

**Fix approach**: Increase alpha of `meterSafe` in the idle gradient for the -24 dB to -60 dB zone. Add a gradient stop at ~60% from top (around -18 dB) with `meterSafe.withAlpha(0.30f)` transitioning to the existing 0.80f at the bottom. This will make the bottom third of idle meter bars visibly blue.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — adjust idle gradient stops (lines 102-121): add intermediate stop at ~60% height with alpha 0.30f for meterSafe, and increase alpha at the -24 dB mid-fill point from 0.08f to 0.18f

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "output-meter-idle.png" && stop_app` → Expected: screenshot shows visible blue segments in lower portion of output meter bars

## Tests
None

## Technical Details
The output meter uses the same `LevelMeter` class as the input meter. Changes here affect both meters, which is correct since both should show the idle blue segments.

The key gradient in `drawChannel()` lines 102-121 creates the idle appearance. Currently the transition from near-transparent to blue happens in a narrow band at the very bottom. Spreading this transition higher (starting at ~-18 dBFS / ~60% from top) will better match the reference.

RMSE for output meter region: 31.6% — this is the single highest-impact region to improve.

## Dependencies
None
