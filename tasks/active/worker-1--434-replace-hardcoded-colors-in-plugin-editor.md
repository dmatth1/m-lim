# Task 434: Replace Hardcoded Colour Literals in PluginEditor.cpp

## Description
`PluginEditor.cpp` uses two raw `juce::Colour` hex literals that are already defined
in `Colours.h`. If a future task adjusts those constants, the editor silently drifts.

- Line 42: `juce::Colour (0xffFFD700)` → should be `MLIMColours::peakLabel`
  (same value as `Colours.h` line 37)
- Line 161: `juce::Colour (0xff1a1a1a)` in `paint()` → should be `MLIMColours::background`
  (same value as `Colours.h` line 8)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — replace two inline hex literals with named constants
Read: `src/ui/Colours.h` — verify constant names and values

## Acceptance Criteria
- [ ] Run: `grep -in "0xffFFD700\|0xff1a1a1a" src/PluginEditor.cpp` → Expected: no output (zero matches)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
In `PluginEditor.cpp`:
- Replace `juce::Colour (0xffFFD700)` with `MLIMColours::peakLabel`
- Replace `juce::Colour (0xff1a1a1a)` with `MLIMColours::background`

Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
