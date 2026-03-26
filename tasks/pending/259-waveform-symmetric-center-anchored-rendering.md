# Task 259: Waveform Display — Symmetric Center-Anchored Rendering

## Description
The Pro-L 2 waveform display renders the input signal as a **symmetrical waveform centered on a
horizontal midline** — peaks extend both upward and downward from the center axis, like an
oscilloscope. This is the standard audio waveform visualization style.

M-LIM's current implementation fills from the **bottom edge upward** (single-direction fill), which
looks different from the reference and fails to communicate the audio signal's symmetrical nature.

Reference evidence: `v1-0005.png` and `v1-0015.png` clearly show waveform spikes extending above
AND below the center axis. The output envelope (`outputWaveform`/`outputEnvelope`) is shown as a
single curve at the center height.

**Implementation approach**:
1. In `WaveformDisplay::drawInputFill()` and `drawOutputFill()`, change from filling the full area
   bottom-to-signal to filling symmetrically from the vertical center:
   - Compute `midY = area.getCentreY()`
   - The signal amplitude `h = midY - levelToY(level, area)` pixels from center
   - Fill the rect `[midY - h, midY + h]` (symmetric about midY)
2. The `levelToY()` function can remain as-is; just mirror the fill direction.
3. `drawGainReduction()` should remain top-anchored (GR fills from the top down) — no change needed.
4. `drawOutputEnvelope()` should remain as a line at `levelToY(outputLevel)` mapped symmetrically
   — draw the line at `midY - amplitude` (top half only, since it represents the peak).

This change only affects `drawInputFill`, `drawOutputFill`, and `drawOutputEnvelope`.
The scale, ceiling line, grid lines, peak markers, and GR fill are all unaffected.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawInputFill()`, `drawOutputFill()`, `drawOutputEnvelope()`
Read:   `src/ui/WaveformDisplay.h` — `kMaxGRdB`, `kScaleWidth`, `levelToY()` signature
Read:   `/reference-docs/video-frames/v1-0005.png` — reference showing symmetric waveform
Read:   `/reference-docs/video-frames/v1-0015.png` — another frame showing waveform style
Read:   `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — full UI reference

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` →
      Expected: build succeeds
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` →
      Expected: all tests pass
- [ ] Visual check: with audio input, the waveform should display peaks going both UP and DOWN from
      the horizontal center, mirroring the reference screenshots

## Tests
None (visual rendering — no unit tests)

## Technical Details
Current `drawFilledWaveformPath` (used by drawInputFill/drawOutputFill) builds a path from the
bottom edge up to the signal level:
```cpp
path.startNewSubPath (x, anchorY);   // anchorY = area.getBottom()
path.lineTo (x, y);                  // y = levelToY(level, area) = top of fill
```

For symmetric rendering, change to:
```cpp
const float midY = area.getCentreY();
// Top path: midY upward
path.startNewSubPath (firstX, midY);
path.lineTo (x, midY - (midY - levelY));   // mirror above center
path.lineTo (lastX, midY);
path.closeSubPath();
// Bottom path: midY downward (mirrored)
path.startNewSubPath (firstX, midY);
path.lineTo (x, midY + (midY - levelY));   // mirror below center
path.lineTo (lastX, midY);
path.closeSubPath();
g.fillPath (path);
```
Or alternatively, pass `midY` as the anchor and draw a symmetric band:
For each column: fill `[midY - halfH, midY + halfH]` where `halfH = midY - levelToY(level, area)`.

The simpler implementation: modify `drawFilledWaveformPath` to accept an `anchor` and `symmetric`
flag. When `symmetric=true`, build two mirrored fills.

For `drawOutputEnvelope`, just change the stroke to follow `midY - (midY - levelToY(...))` so it
traces the top of the symmetric fill.

## Dependencies
None
