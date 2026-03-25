# Task 058: Input Gain Slider as Waveform Left-Edge Overlay

## Description
Task 047 places Input Gain and Output Ceiling as vertical sliders inside the bottom control strip. This is WRONG for Pro-L 2 parity. In Pro-L 2, the Input Gain is a tall vertical slider overlaid on the LEFT edge of the waveform display area (not in the control strip), with a "GAIN" label above it and a yellow/gold floating tooltip showing the current value (e.g., "+10.5"). The Output Ceiling control remains in the bottom-right area. This task corrects the Input Gain slider placement.

Reference: See `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (left side showing "GAIN" label with "+10.5"), `/reference-docs/video-frames/v1-0020.png` (GAIN slider visible on left), `/reference-docs/video-frames/v2-0070.png`.

## Produces
None

## Consumes
EditorCore
ColoursDefinition
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — add vertical gain slider as overlay on waveform left edge
Modify: `M-LIM/src/PluginEditor.cpp` — position gain slider overlaying waveform left edge in resized()
Modify: `M-LIM/src/ui/ControlStrip.h` — remove input gain from control strip layout (it belongs on waveform edge)
Modify: `M-LIM/src/ui/ControlStrip.cpp` — remove input gain from control strip
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "inputGain\|GAIN" M-LIM/src/PluginEditor.cpp` → Expected: at least 2 (gain slider in editor, not control strip)
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-058-after.png" && stop_app` → Expected: gain slider visible on left edge of waveform area

## Tests
None (visual layout — verified by UI parity auditor)

## Technical Details
- The input gain slider is a TALL vertical slider (~the full height of the waveform area) positioned on the left edge of the waveform display, partially overlaying it
- Slider width: ~30-40px
- "GAIN" label text above the slider in caps (textSecondary color)
- Yellow/gold floating tooltip (#FFD700) showing the current dB value when hovering/dragging
- Slider range: -12 to +36 dB, default 0
- Visual style: thin dark track, subtle fill, small thumb marker
- The slider should be a child of the editor (not the waveform), positioned to overlap the waveform's left edge
- APVTS attachment to "inputGain" parameter
- Task 047's description of placing gain in the control strip should be superseded by this task

## Dependencies
Requires task 027, supersedes task 047's input gain placement
