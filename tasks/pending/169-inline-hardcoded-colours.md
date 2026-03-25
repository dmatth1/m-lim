# Task 169: Move Inline Hardcoded Colours into Colours.h

## Description
Several UI files hardcode `juce::Colour(0xRRGGBBAA)` literals inline instead of using the named constants in `Colours.h`. This breaks the central theming contract: if the dark palette changes, developers must hunt for scattered hex literals instead of updating one file.

Known occurrences (verify exact line numbers by reading the files):
- `LookAndFeel.cpp`: `juce::Colour(0xff2A2A2A)` used for a background colour that should match `MLIMColours::displayBackground` or a new named constant
- `WaveformDisplay.cpp`: at least two unnamed colours — a grid line colour (`0xff2E3040`) and a hover/selection overlay (`0x30FFFFFF`)
- `ControlStrip.cpp`: `juce::Colour(0xff2A2A2A)` for a combo box background

The fix is to:
1. Add the missing named constants to `Colours.h`
2. Replace the inline hex literals with the named constants

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/Colours.h` — existing named constants; add missing ones here
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — replace inline hex with named constant
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — replace inline hex with named constants
Modify: `M-LIM/src/ui/ControlStrip.cpp` — replace inline hex with named constant

## Acceptance Criteria
- [ ] Run: `grep -rn "juce::Colour(0x" M-LIM/src/ui/LookAndFeel.cpp M-LIM/src/ui/WaveformDisplay.cpp M-LIM/src/ui/ControlStrip.cpp` → Expected: no matches (all hex literals replaced with named constants)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
Scan each file for `juce::Colour(0x` literals. For each one:
1. Check whether the colour is semantically identical to an existing constant in `Colours.h` — if so, use that constant.
2. If it is a distinct colour not yet named, add a well-named constant to `Colours.h` (e.g. `waveformGridLine`, `waveformHoverOverlay`) before replacing the literal.

Do not change any colour values — this is a naming/organisation task only. The visual result must be pixel-identical before and after.

## Dependencies
None
