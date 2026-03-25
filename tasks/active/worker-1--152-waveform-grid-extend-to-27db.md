# Task 152: Extend Waveform dB Grid Lines to -27 dB

## Description
The waveform display grid lines are defined in `kGridDB[]` as:
```cpp
static const float kGridDB[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f, -15.0f, -18.0f, -21.0f, -24.0f };
```
But `kMaxGRdB = 30.0f`, so the display extends from 0 dB to -30 dB. The bottom 6 dB of the display (-24 to -30 dB) has no grid line, leaving a visual gap. The reference Pro-L 2 shows grid lines extending to at least -27 dB.

Fix: Add `-27.0f` to `kGridDB` so the grid covers the full visible range uniformly.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — extend `kGridDB` array at line 6

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "task-141-after.png" && stop_app` → Expected: screenshot shows a horizontal grid line at the -27 dB position in the waveform display, filling the visual gap between -24 dB and the bottom
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c error` → Expected: `0`

## Tests
None

## Technical Details
- `src/ui/WaveformDisplay.cpp:6-7`: add `-27.0f` to the `kGridDB` array:
  ```cpp
  static const float kGridDB[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f,
                                   -15.0f, -18.0f, -21.0f, -24.0f, -27.0f };
  ```
- The `drawScale()` function already uses `kGridDB` for label positions, so the -27 label will also appear automatically.

## Dependencies
None
