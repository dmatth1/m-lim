# Task 032: Waveform Display Mode Selector UI Control

## Description
Pro-L 2 has a clickable display mode selector in the waveform display area that cycles between Fast/Slow/SlowDown/Infinite/Off modes. Task 022 defines the display modes but no UI control exists to switch between them. Add a small mode selector widget in the waveform display area.

## Produces
None

## Consumes
WaveformDisplayInterface
ColoursDefinition

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.h` — add mode selector child component or clickable label
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — implement mode selector rendering and click handling
Read: `M-LIM/src/ui/Colours.h` — color constants

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "DisplayMode" M-LIM/src/ui/WaveformDisplay.h` → Expected: at least 2 (enum + getter/setter)

## Tests
None (visual component)

## Technical Details
- Small text label in top-left or bottom-left of waveform area showing current mode name
- Click to cycle through modes: Fast → Slow → SlowDown → Infinite → Off → Fast
- Right-click shows popup menu with all mode options
- Text style: small, textSecondary color, slightly brighter on hover
- Current mode should be stored and applied to the waveform scroll speed
- In Pro-L 2 this appears as a subtle text label, not a prominent control

## Dependencies
Requires task 022
