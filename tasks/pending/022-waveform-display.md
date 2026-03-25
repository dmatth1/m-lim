# Task 022: Waveform Display Component

## Description
Create the main real-time waveform display — the centerpiece of the UI. Shows scrolling waveform history with layered input level (cyan), output level (dark blue), gain reduction overlay (red), envelope curves, and peak marker labels.

## Produces
Implements: `WaveformDisplayInterface`

## Consumes
ColoursDefinition
MeterDataInterface

## Relevant Files
Create: `M-LIM/src/ui/WaveformDisplay.h` — class declaration
Create: `M-LIM/src/ui/WaveformDisplay.cpp` — implementation
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `M-LIM/src/dsp/MeterData.h` — MeterData struct (standalone header, no heavy DSP includes needed)
Read: `SPEC.md` — WaveformDisplayInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component — verified by UI parity auditor)

## Technical Details
- Ring buffer stores waveform history (several seconds of MeterData snapshots)
- Display modes: Fast (steady scroll), Slow (slower scroll), SlowDown (hybrid), Infinite (accumulates), Off
- Paint layers (back to front):
  1. Dark background with subtle horizontal grid lines at dB intervals
  2. Output level filled area (dark blue, semi-transparent)
  3. Input level filled area (light cyan, semi-transparent)
  4. Gain reduction bars from top (bright red)
  5. Envelope/release curve overlay (tan/light brown lines)
  6. Peak markers: yellow/gold labels showing "-X.X dB" at GR peaks
- Vertical dB scale on right edge: 0, -3, -6, -9, -12, -15, -18, -21, -24 dB
- pushMeterData() called from editor timer, appends to ring buffer
- Scrolling: new data enters from right, old data scrolls left
- Peak detection: find local maxima in GR data, render labels above peaks
- 60fps repaint via Timer (inherited from editor timer callback)
- Efficient painting: use juce::Path for waveform fills, avoid per-pixel operations

## Dependencies
Requires task 003
