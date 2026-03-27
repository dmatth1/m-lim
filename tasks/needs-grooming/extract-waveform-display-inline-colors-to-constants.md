# Task: Extract WaveformDisplay inline colour literals to Colours.h constants

## Description
`WaveformDisplay.cpp` contains four raw hex colour literals that are constructed
inline inside `paint()`-path functions. They are not referenced in `Colours.h`
and have no symbolic names, making them invisible to future colour-tuning tasks.

Locations:
- Line 337: `juce::Colour midFill { 0xff828AA5 }` — idle waveform mid fill
- Line 362: `juce::Colour cCol { 0xff828AA5 }` — ceiling line colour (same value, duplicated)
- Line 385: `juce::Colour lFill { 0xff9898A8 }` — leveling waveform fill
- Line 435: `juce::Colour (0xffD8ACD0).withAlpha (0.52f)` — left-edge gradient stop

The value on line 337 and 362 are identical (`0xff828AA5`), so there are really
three distinct colours, two of which share the same magic number. Extracting them
removes the duplication and makes them tunable from a single place.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — add four named constants in the "Waveform display" section
Modify: `src/ui/WaveformDisplay.cpp` — replace inline literals with the new constants
Read: `src/ui/WaveformDisplay.h` — understand the component structure

## Acceptance Criteria
- [ ] Run: `grep "0xff828AA5\|0xff9898A8\|0xffD8ACD0" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: no output
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
Suggested constant names (add to `Colours.h` waveform section):
```cpp
const juce::Colour waveformIdleMidFill   { 0xff828AA5 };  // idle mid-fill and ceiling colour
const juce::Colour waveformLevelingFill  { 0xff9898A8 };  // leveling waveform fill colour
const juce::Colour waveformLeftEdgeStop  { 0xffD8ACD0 };  // left-edge gradient bottom stop (combine with .withAlpha(0.52f) at use site)
```

No behaviour change — purely a code-quality / maintainability fix.

## Dependencies
None
