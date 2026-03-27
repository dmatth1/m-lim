# Task: Waveform Gradient Top — Slight Warm Shift

## Description

Pixel analysis at the very top of the waveform display (y=50, x=300) shows a minor mismatch:

- M-LIM at y=50: srgb(42, 39, 47) — slightly cool/neutral dark purple
- Reference at y=50: srgb(38, 33, 39) — warmer dark (has more red relative to blue)
- Delta: M-LIM is +4R, +6G, +8B too cool/bright

The current displayGradientTop = #28242A = (40, 36, 42). The reference top corresponds
to approximately (38, 33, 39) which is warmer (red-biased) and slightly darker.

**Fix**: Adjust displayGradientTop from #28242A to a slightly warmer value:
- Candidate: #26201E = (38, 32, 30) — too warm/too dark in blue
- Candidate: #261E22 = (38, 30, 34) — slightly warmer, reduced blue
- Candidate: #271F22 = (39, 31, 34) — closest to target (38, 33, 39)

**Note**: This is a small adjustment. Only implement if the RMSE improves — the top area
currently has only a ~5-8 unit gap vs reference, which is already close.

## Relevant Files

Modify: `src/ui/Colours.h` — displayGradientTop constant

## Acceptance Criteria

- [ ] Run wave RMSE → Expected: ≤ 16.72% (must not get worse)
- [ ] Run full RMSE → Expected: ≤ 19.46%
- [ ] Save results to `screenshots/task-NNN-rmse-results.txt`

## Tests
None

## Technical Details

**Current**: `const juce::Colour displayGradientTop { 0xff28242A };`
**Try**: `const juce::Colour displayGradientTop { 0xff261E22 };`

This changes:
- R: 0x28=40 → 0x26=38 (−2)
- G: 0x24=36 → 0x1E=30 (−6)
- B: 0x2A=42 → 0x22=34 (−8)

The reference at the very top shows srgb(38,33,39) — our target.

Also try #271F26 = (39,31,38) as alternative (keeps B higher).

## Dependencies
None
