# Task: Enlarge Rotary Knobs to Match Reference Size

## Description
The rotary knobs in the control strip are too small compared to Pro-L 2 reference. Our knobs render at approximately 36px diameter (knob row height 56px minus 20px for label+value text), while the reference knobs appear at approximately 50-55px diameter, filling more of the vertical space. The knobs should be larger and more visually prominent to match the reference's proportions.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — increase `kKnobRowH` from 56 to approximately 68-72, giving knobs ~48-52px height
Modify: `src/PluginEditor.h` — increase `kControlStripH` from 92 to approximately 104-108 to accommodate larger knob row
Modify: `src/ui/RotaryKnob.cpp` — verify paint() method scales correctly with larger bounds; `labelH` and `valueH` at 10px each may need slight reduction or the knob face ratio increased
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — close-up of reference knobs showing size proportions

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot comparison of knob row → Expected: knobs visually larger and closer to reference proportions

## Tests
None

## Technical Details
- Current layout: kControlStripH=92, kKnobRowH=56, knob face≈36px diameter
- Target: kKnobRowH≈68-72, knob face≈48-52px diameter
- Increasing kControlStripH reduces waveform display height by the same amount; the waveform is currently 368px (73.6% of 500px), and reducing it by ~12-16px to ~352-356px (70.4-71.2%) still matches the reference proportion
- The control strip currently occupies 18.4% of total height vs reference ~15%; making it slightly taller for bigger knobs increases this to ~20-21.6%. To compensate, consider reducing kTopBarH from 40 to 32-36, which would bring the control strip proportion closer to reference while providing more space for knobs.
- Alternative approach: keep kControlStripH the same but reduce label/value text heights to 8px each, giving knobs 40px diameter. This is simpler but may not match reference fully.

## Dependencies
None
