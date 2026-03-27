# Task: Reduce Waveform Grid Line Alpha for More Subtle Appearance

## Description
The horizontal dB grid lines in the waveform display are drawn at `waveformGridLine.withAlpha(0.6f)`,
where `waveformGridLine = 0xff9AA0B4` (light blue-gray). This creates very prominent horizontal
stripes across the empty waveform — especially visible in the idle (no audio) state.

In the reference (FabFilter Pro-L 2 with active audio), the grid lines are obscured by dense
waveform content and appear much more subtle. The M-LIM grid lines are one of the most
visible differences in the waveform region when comparing the idle screenshot.

Reducing the alpha from `0.6f` to `0.35f` would keep the grid lines readable but make them
significantly more subtle, closer to the reference's barely-visible grid.

**Expected RMSE gain:** Wave region −0.2pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method, grid line alpha
Read: `src/ui/Colours.h` — `waveformGridLine = 0xff9AA0B4`

## Acceptance Criteria
- [ ] Run: build + screenshot + RMSE → Expected: Wave RMSE ≤ 19.3% (down from 19.44%)
- [ ] Run: visual check → Expected: horizontal dB grid lines are visible but subtle; not harsh bright stripes
- [ ] Run: Full RMSE → Expected: ≤ 21.1% (no regression)

## Tests
None

## Technical Details

In `src/ui/WaveformDisplay.cpp`, `drawBackground()`, locate:

```cpp
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.6f));
```

Change to:

```cpp
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.35f));
```

**Rationale:** The reference waveform shows subtle grid lines at approximately 15-20%
opacity due to the dense audio waveform fills sitting on top. M-LIM's idle state has
no waveform fills, so the grid lines appear at full specified opacity. Reducing from
0.6 to 0.35 keeps labels readable while approaching the reference's apparent grid
line brightness.

## Dependencies
None
