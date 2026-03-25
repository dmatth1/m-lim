# Task 073: Loudness Target Level Selector

## Description
Pro-L 2 has a loudness target selector that lets users pick standard target levels (-9 LUFS for CD, -14 LUFS for Streaming, -23 LUFS for EBU R128, -24 LUFS for ATSC A/85, or Custom). This target is used to color-code the loudness histogram bars and as a reference line. No existing task covers this dropdown selector or the target level parameter.

Reference: See `/reference-docs/video-frames/v1-0010.png` (dropdown showing target level options), `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (right side showing "Strm +9" and "-14 (Strm)" indicators).

## Produces
None

## Consumes
LoudnessPanelInterface
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.h` — add target level selector and storage
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — implement dropdown menu and target rendering
Modify: `M-LIM/src/Parameters.h` — add "loudnessTarget" parameter (choice or float)
Modify: `M-LIM/src/Parameters.cpp` — add loudnessTarget to parameter layout
Read: `M-LIM/src/ui/Colours.h` — color constants

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "loudnessTarget" M-LIM/src/Parameters.h` → Expected: at least 1 match
- [ ] Run: `grep -c "target\|Target" M-LIM/src/ui/LoudnessPanel.cpp` → Expected: at least 3 matches

## Tests
None (visual component + parameter addition)

## Technical Details
- Add a new parameter "loudnessTarget" as a choice parameter with options:
  - "-9 LUFS (CD)"
  - "-14 LUFS (Streaming)" (default)
  - "-23 LUFS (EBU R128)"
  - "-24 LUFS (ATSC A/85, TR-B32)"
  - "Custom..."
- Clicking opens a popup menu with these options
- "Custom..." opens a text input or slider to set a custom target level
- The selected target level is displayed as a highlighted indicator on the loudness histogram (e.g., "-14 (Strm)" label)
- Also shown as "Strm +9" type label indicating the scale range
- The target value is used by the loudness histogram (task 057) to color-code bars: white below target, yellow near target, red above target
- Visual: small clickable label/button in the loudness panel area, dark background, subtle text

## Dependencies
Requires task 023 and task 057
