# Task 272: RMSE Re-Measure After Visual Parity Tasks 204/205/206/270/271

## Description
Rebuild and re-measure RMSE against the Pro-L 2 reference after the visual parity tasks complete:
- Task 204: algorithm selector single button
- Task 205: ADVANCED vertical side tab
- Task 206: knob face flat disc appearance
- Task 270: waveform dB scale labels restored as left-edge overlay
- Task 271: level meter segment texture on empty track

Use the same methodology as task 256/268 to produce a comparable RMSE number.

### Methodology (from task 256):
1. Build standalone: `cmake --build build --target MLIM_Standalone -j$(nproc)`
2. Launch on Xvfb :99, capture screenshot with `scrot` or `import`
3. Crop to plugin area (excluding desktop/title bar): detect window bounds, crop plugin content
4. Resize crop to match reference dimensions
5. Compare with `compare -metric RMSE <current-crop> /reference-docs/reference-screenshots/prol2-main-ui.jpg`
6. Report full-image RMSE and per-region RMSE (waveform area, control strip, loudness panel)

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/` — check for any existing screenshot helper scripts
Read: `screenshots/task-256-rmse-results.txt` — previous methodology and baseline numbers

## Acceptance Criteria
- [ ] Run: RMSE measurement completes and produces a numeric result
- [ ] Run: Report full-image RMSE and confirm whether improvement over 26.6% baseline (task 268)
- [ ] Run: commit a `screenshots/task-272-rmse-results.txt` with methodology + results

## Tests
None

## Technical Details
Previous baselines:
- Task 256 (pre-gradient fixes): 31.4% full-image RMSE
- Task 264: 25.4% RMSE
- Task 268 (after 265-267): 26.6% RMSE (slight regression from gradient change)

The per-region breakdown helps identify the largest remaining contributors.

## Dependencies
Requires tasks 204, 205, 206, 270, 271
