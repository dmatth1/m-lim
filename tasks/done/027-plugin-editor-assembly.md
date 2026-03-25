# Task 027: Plugin Editor Assembly

## Description
Wire up the PluginEditor to compose all UI components (TopBar, WaveformDisplay, LevelMeters, GainReductionMeter, LoudnessPanel, ControlStrip) into the final layout. Implement the 60fps timer that reads MeterData from the processor FIFO and updates all meters/displays.

## Produces
Implements: `EditorCore`

## Consumes
TopBarInterface
WaveformDisplayInterface
LevelMeterInterface
LoudnessPanelInterface
ControlStripInterface
MeterDataInterface
PluginProcessorCore

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — add component members, timer
Modify: `M-LIM/src/PluginEditor.cpp` — implement resized(), timerCallback(), paint()
Read: `M-LIM/src/ui/TopBar.h` — top bar
Read: `M-LIM/src/ui/WaveformDisplay.h` — waveform display
Read: `M-LIM/src/ui/LevelMeter.h` — level meters
Read: `M-LIM/src/ui/GainReductionMeter.h` — GR meter
Read: `M-LIM/src/ui/LoudnessPanel.h` — loudness panel
Read: `M-LIM/src/ui/ControlStrip.h` — control strip
Read: `M-LIM/src/PluginProcessor.h` — processor/meter FIFO access

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "addAndMakeVisible" M-LIM/src/PluginEditor.cpp` → Expected: at least 6 (all major components added)

## Tests
None (UI assembly — visual verification)

## Technical Details
- Layout proportions (matching Pro-L 2):
  - TopBar: full width, 30px height at top
  - Input LevelMeter: ~20px wide, LEFT of waveform (Pro-L 2 has input meter on left)
  - WaveformDisplay: ~70-75% of remaining width, CENTER
  - GainReductionMeter + Output LevelMeter: ~40-50px wide, RIGHT of waveform
  - LoudnessPanel: collapsible, adjacent to or overlaying right side
  - ControlStrip: full width, ~120px height at bottom
- Default window size: 900x500
- Resizable: yes, with min 600x350, aspect ratio ~1.8:1
- setResizable(true, true) with constrainer
- Timer: startTimerHz(60) in constructor, stopTimer in destructor
- timerCallback: drain MeterData FIFO, update all meter/display components
- Wire TopBar callbacks to processor (undo, redo, A/B, presets)
- Wire ControlStrip APVTS attachments in constructor
- Apply MLIMLookAndFeel in constructor, reset in destructor
- HiDPI: use setScaleFactor or Desktop::getDefaultLookAndFeel scaling

## Dependencies
Requires tasks 020, 021, 022, 023, 024, 025, 026
