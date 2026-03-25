# Task 059: Knob Graduation Tick Marks for Pro-L 2 Parity

## Description
Pro-L 2 knobs have subtle graduation/tick marks around the outer circumference of the knob face, similar to a clock or dial face. These small marks add professional polish and help users gauge the knob position. Task 018 (RotaryKnob) and task 003 (LookAndFeel) do not specify these tick marks. This is visible in the close-up reference at `/reference-docs/reference-screenshots/prol2-features.jpg`.

Reference: See `/reference-docs/reference-screenshots/prol2-features.jpg` (close-up of LOOKAHEAD, ATTACK, RELEASE, CHANNEL LINKING knobs showing graduation marks around edges).

## Produces
None

## Consumes
RotaryKnobInterface
LookAndFeelDefinition

## Relevant Files
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — add tick marks in drawRotarySlider override
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — reference close-up

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "tick\|graduation\|mark" M-LIM/src/ui/LookAndFeel.cpp` → Expected: at least 2 matches

## Tests
None (visual detail)

## Technical Details
- In `drawRotarySlider()`, draw small tick marks around the outer edge of the knob circle
- Approximately 20-30 evenly-spaced marks around the full arc range (270 degrees)
- Tick mark color: very subtle gray, slightly lighter than the knob background (~#555555, alpha 0.4)
- Tick mark size: 1-2px wide, 3-4px long, radiating outward from center
- Major tick marks at 0%, 25%, 50%, 75%, 100% positions could be slightly longer/brighter
- Marks should be drawn BEHIND the value arc but above the knob face background
- Reference prol2-features.jpg shows very subtle but visible marks creating a "dial" appearance

## Dependencies
Requires task 003 (LookAndFeel) and task 018 (RotaryKnob)
