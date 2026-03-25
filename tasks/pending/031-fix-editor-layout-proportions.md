# Task 031: Fix Editor Layout Proportions for Pro-L 2 Parity

## Description
The editor assembly (task 027) specifies ~65% waveform / ~35% right panel, but Pro-L 2 uses approximately 70-75% for the waveform display with meters flanking it — input meter on the LEFT and output meter on the RIGHT of the waveform. The current plan puts all meters on the right side, which is incorrect. Fix the layout to match Pro-L 2's actual arrangement.

## Produces
None

## Consumes
EditorCore
LevelMeterInterface
WaveformDisplayInterface
GainReductionMeter

## Relevant Files
Modify: `M-LIM/src/PluginEditor.cpp` — fix resized() layout calculations
Modify: `M-LIM/src/PluginEditor.h` — adjust component member organization if needed
Read: `SPEC.md` — EditorCore interface for layout spec

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -A5 "resized" M-LIM/src/PluginEditor.cpp | head -20` → Expected: shows layout code with input meter left, waveform center, output meter + GR right

## Tests
None (visual layout — verified by UI parity auditor)

## Technical Details
- Pro-L 2 layout (left to right):
  - Input level meter: thin vertical bar (~20px wide) on LEFT edge
  - Waveform display: ~70-75% of remaining width, center
  - Gain reduction meter + Output level meter: RIGHT edge (~40-50px wide total)
  - Loudness panel: collapsible, overlays or sits adjacent to waveform area
- The input meter and output meter must be on OPPOSITE sides of the waveform
- TopBar stays full width at top (~30px)
- ControlStrip stays full width at bottom (~120px)
- GR meter should be immediately right of the waveform, with output meter to its right

## Dependencies
Requires task 027
