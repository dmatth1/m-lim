# Task 003: Colour Constants and LookAndFeel

## Description
Create the colour constants header and custom LookAndFeel class that defines M-LIM's dark theme. This provides the visual foundation all UI components depend on.

## Produces
Implements: `ColoursDefinition`
Implements: `LookAndFeelDefinition`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/ui/Colours.h` — namespace MLIMColours with all color constants
Create: `M-LIM/src/ui/LookAndFeel.h` — MLIMLookAndFeel class declaration
Create: `M-LIM/src/ui/LookAndFeel.cpp` — LookAndFeel implementation with overrides
Read: `SPEC.md` — ColoursDefinition and LookAndFeelDefinition interfaces

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors
- [ ] Run: `grep -c "0x" M-LIM/src/ui/Colours.h` → Expected: at least 15 (all colors defined)

## Tests
None (visual constants, no testable logic)

## Technical Details
- All colors as `juce::Colour` constants in `MLIMColours` namespace
- Colors from SPEC.md ColoursDefinition interface — use exact hex values
- LookAndFeel overrides: drawRotarySlider (dark circle + cyan arc + white pointer), drawComboBox (dark dropdown), drawButtonBackground (dark button with hover)
- Set default component colors (window background, text, etc.) in constructor
- Use system sans-serif font

## Dependencies
Requires task 001
