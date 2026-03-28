# Task: Document and standardize UI repaint trigger strategy

## Description
UI components use inconsistent repaint strategies:
- LoudnessPanel, LevelMeter, GainReductionMeter: call `repaint()` in every setter method
- WaveformDisplay: relies on timer-driven repaint, setters don't call repaint()
- RotaryKnob: calls repaint() in setters

For meter components that update at 60fps via timer, calling `repaint()` in setters is redundant since the timer already triggers repaints. This creates unnecessary repaint coalescing overhead.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — remove redundant repaint() calls from data setters (values set from timerCallback)
Modify: `M-LIM/src/ui/LevelMeter.cpp` — remove redundant repaint() from meter data setters
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — remove redundant repaint() from meter data setters
Read: `M-LIM/src/PluginEditor.cpp` — verify timerCallback drives all meter updates

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (performance optimization, no visible behavior change)

## Technical Details
The PluginEditor::timerCallback() updates all meter components then calls repaint on the editor. Individual setter repaint() calls are thus redundant for data that flows through the timer path. Remove `repaint()` from:
- LoudnessPanel: setMomentary(), setShortTerm(), setIntegrated(), setRange()
- LevelMeter: setLevel(), setPeakHold()
- GainReductionMeter: setGainReduction(), setPeakGR()

Keep repaint() in setters that respond to user interaction (e.g., setDisplayMode, button clicks).

## Dependencies
None
