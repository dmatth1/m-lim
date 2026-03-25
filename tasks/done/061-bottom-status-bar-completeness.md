# Task 061: Bottom Status Bar Complete Layout for Pro-L 2 Parity

## Description
The bottom status bar in Pro-L 2 contains several UI elements that are missing or underspecified in task 025 (ControlStrip). The actual bottom bar layout includes: "MIDI Learn" dropdown, "True Peak Limiting" toggle with green/red indicator, "Oversampling: Xx" display, "Dither: XX Bits (X)" display, then small toggle buttons (TP, waveform mode icon, Loudness), pause "||" button, measurement mode selector (Short Term/Momentary/Integrated), and "Out: X.X dBTP" readout on the far right.

Reference: See `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (bottom bar), `/reference-docs/video-frames/v1-0005.png`, `v1-0025.png`, `v1-0040.png`.

## Produces
None

## Consumes
ControlStripInterface
ColoursDefinition

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.h` — add missing bottom bar elements
Modify: `M-LIM/src/ui/ControlStrip.cpp` — implement complete bottom bar layout
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "dBTP\|midiLearn\|truePeak" M-LIM/src/ui/ControlStrip.cpp` → Expected: at least 3 matches

## Tests
None (visual layout)

## Technical Details
The bottom status bar (below the knob row) should contain, left to right:

1. **"MIDI Learn"** — dropdown/button (shows "MIDI Learn" text with small dropdown arrow). Clicking opens MIDI learn mode.
2. **"True Peak Limiting"** — toggle button with green indicator dot when enabled, red/gray when disabled
3. **"Oversampling: Xx"** — text label showing current oversampling factor (e.g., "Oversampling: 8x" or "Off")
4. **"Dither: XX Bits (X)"** — text label showing dither config (e.g., "Dither: 16 Bits (O)" where O = Optimized)
5. **Separator/spacer**
6. **"TP"** toggle button — toggles true peak indicator on waveform display
7. **Waveform mode icon** — small icon button to cycle display modes (or show waveform)
8. **"Loudness"** toggle button — toggles loudness meter panel visibility
9. **"||"** pause button — pauses loudness measurement
10. **Measurement mode selector** — "Short Term" / "Momentary" / "Integrated" as clickable text
11. **"Out: X.X dBTP"** — output level readout, far right

Layout: small font (10-11px), dark background matching panel, items separated by subtle spacing. Each toggle should have a hover highlight and active state indicator.

## Dependencies
Requires task 025
