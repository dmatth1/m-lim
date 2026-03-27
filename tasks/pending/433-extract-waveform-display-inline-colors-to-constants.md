# Task 433: Extract WaveformDisplay Inline Colour Literals to Colours.h Constants

## Description
`WaveformDisplay.cpp` contains four raw hex colour literals constructed inline inside
paint-path functions. They have no symbolic names and are invisible to colour-tuning tasks.

Locations:
- Line 337: `juce::Colour midFill { 0xff828AA5 }` — idle waveform mid fill (steel blue)
- Line 362: `juce::Colour cCol { 0xff828AA5 }` — center tent (same value, duplicated)
- Line 385: `juce::Colour lFill { 0xff9898A8 }` — leveling waveform fill (neutral)
- Line 435: `juce::Colour (0xffD8ACD0).withAlpha(0.52f)` — left-edge gradient stop (pink)

Lines 337 and 362 share the same value — extract as one constant.

Add to `Colours.h` in the "Waveform display" section:
```cpp
const juce::Colour waveformIdleMidFill   { 0xff828AA5 };  // mid/center tent idle fill
const juce::Colour waveformIdleLowFill   { 0xff9898A8 };  // lower idle fill
const juce::Colour waveformLeftEdgeTint  { 0xffD8ACD0 };  // left-edge gradient tint
```

Then replace all inline usages in `WaveformDisplay.cpp` with the new constant names.

Note: If task 423 (left-edge colour correction) removes the `0xffD8ACD0` usage entirely,
still add `waveformLeftEdgeTint` as a documented constant so future tuning is discoverable,
but mark it `// currently unused` if the overlay was removed.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — add three named constants in the "Waveform display" section
Modify: `src/ui/WaveformDisplay.cpp` — replace four inline hex literals with named constants
Read: `src/ui/WaveformDisplay.h` — understand component structure

## Acceptance Criteria
- [ ] Run: `grep -rn "0xff828AA5\|0xff9898A8\|0xffD8ACD0" src/ui/WaveformDisplay.cpp` → Expected: no output (zero matches)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
Requires task 423
