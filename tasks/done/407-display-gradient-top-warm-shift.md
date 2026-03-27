# Task 407: displayGradientTop — Warm Shift to Match Reference Top Zone

## Description

Pixel analysis of the top waveform zone shows M-LIM is slightly too cool and slightly
too bright compared to the reference:

**Broad zone analysis (wave22 audit, crop 650x100+0+10 from 900x500):**
- M-LIM avg: (47, 46, 53)
- Reference avg: (52, 44, 47)
- Delta: M-LIM is -5R, +2G, +6B — too blue and slightly dark in red

**Point analysis at y=50, x=300:**
- M-LIM: srgb(42, 39, 47)
- Reference: srgb(38, 33, 39)
- Delta: M-LIM is +4R, +6G, +8B too cool/bright

The current `displayGradientTop = 0xff28242A` = (40, 36, 42). The reference top corresponds
to approximately (38, 33, 39) — warmer (red-biased) and slightly darker/less blue.

**Fix**: Adjust `displayGradientTop` to a slightly warmer, less-blue value.

Recommended candidates (in priority order):
1. `0xff271F22` = (39, 31, 34) — closest to point target (38, 33, 39)
2. `0xff261E22` = (38, 30, 34) — slightly warmer, reduced blue
3. `0xff342C2F` = (52, 44, 47) — matches zone avg exactly (may overshoot point)

Try option 1 first and measure RMSE; if wave RMSE doesn't improve, try option 3.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientTop` constant
Read: `src/ui/WaveformDisplay.cpp` — verify gradient rendering direction (top→bottom confirmed)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: crop 650x100+0+10 from M-LIM 900x500 screenshot, compute avg → Expected: avg R closer to 52 (was 47), B closer to 47 (was 53)
- [ ] Run full RMSE → Expected: Wave RMSE does not worsen vs 19.31% baseline
- [ ] Run: `ls screenshots/task-407-rmse-results.txt` → Expected: file exists

## Tests
None

## Technical Details

**Current**: `const juce::Colour displayGradientTop { 0xff28242A };`

Try in order:
- `0xff271F22` → R:39, G:31, B:34 (−1R, −5G, −8B from current)
- `0xff261E22` → R:38, G:30, B:34 (−2R, −6G, −8B)
- `0xff342C2F` → R:52, G:44, B:47 (broader zone match)
- `0xff271F26` → R:39, G:31, B:38 (keeps B higher)

Pick whichever gives best wave RMSE without worsening full RMSE.

**RMSE methodology**: see task 403 for commands.

## Dependencies
None
