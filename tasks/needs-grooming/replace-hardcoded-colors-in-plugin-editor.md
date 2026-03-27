# Task: Replace hardcoded color literals in PluginEditor.cpp

## Description
`PluginEditor.cpp` uses two raw `juce::Colour` hex literals that are already
defined in `Colours.h`. If a future task adjusts those colour constants, the
editor's paint function silently drifts out of sync.

- Line 42: `juce::Colour (0xffFFD700)` should be `MLIMColours::peakLabel`
  (same value, already in Colours.h line 37)
- Line 161: `juce::Colour (0xff1a1a1a)` in `paint()` should be
  `MLIMColours::background` (same value, Colours.h line 8)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — replace two inline hex literals with named constants
Read: `src/ui/Colours.h` — verify constant names and values

## Acceptance Criteria
- [ ] Run: `grep "0xff1a1a1a\|0xffFFD700\|0xFF1A1A1A\|0xFFFFD700" M-LIM/src/PluginEditor.cpp` → Expected: no output (zero matches)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
- `MLIMColours::background` = `0xff1A1A1A` (same bits as `0xff1a1a1a`)
- `MLIMColours::peakLabel`  = `0xffFFD700`
- No behaviour change — purely a code-quality / maintainability fix

## Dependencies
None
