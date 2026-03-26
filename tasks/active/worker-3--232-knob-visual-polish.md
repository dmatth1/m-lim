# Task 232: Rotary Knob Visual Polish — Remove Tick Marks, Clean Look

## Description
The current RotaryKnob renders 25 graduation tick marks around the knob perimeter. FabFilter Pro-L 2's knobs have a clean, smooth look with NO outer tick marks visible — just a dark knob face, a white pointer line, and a subtle arc track. Remove the tick marks to match Pro-L 2's aesthetic.

Also fine-tune:
- The knob face radius: increase from `radius * 0.78f` to `radius * 0.80f` (slightly larger face)
- The track arc: make it thinner (2.5f → 2.0f stroke width) and slightly darker
- The value arc: keep blue but use `knobArc` color and slightly thinner stroke (3.0f → 2.5f)
- The pointer: current is `pointerLength = faceRadius * 0.55f` — reduce to `0.50f` so it doesn't extend too close to the arc

Additionally, ensure the label font uses a bold weight to match Pro-L 2's control strip label appearance:
- Label text (above knob): use `juce::Font(kFontSizeSmall, juce::Font::bold)` instead of kFontSizeLarge
- Value text (below knob): use `juce::Font(kFontSizeMedium)` — keep readable

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/RotaryKnob.cpp` — remove tick mark drawing block, adjust dimensions
Read: `src/ui/Colours.h` — color constants in use
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — close-up of Pro-L 2 knobs

## Acceptance Criteria
- [ ] Run: `grep -c "numTicks\|isMajor\|kMajor" /workspace/M-LIM/src/ui/RotaryKnob.cpp` → Expected: 0 (tick mark code removed)
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0

## Tests
None

## Technical Details
- Remove the entire "Graduation tick marks" block (lines using numTicks, isMajor, etc.)
- The pointer rectangle should start at `-faceRadius` from center and have length `faceRadius * 0.50f`
- Track arc uses `juce::PathStrokeType(2.0f, ...)` instead of 3.0f
- Value arc uses `juce::PathStrokeType(2.5f, ...)` instead of 3.0f

## Dependencies
None
