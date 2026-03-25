# Task 191: WaveformDisplay — Scale Labels Format and Ceiling Label Spacing

## Description
Two minor formatting issues in `WaveformDisplay`'s dB scale labels, identified by comparing the running plugin screenshot against the reference:

### Issue A: Ceiling label has no space before "dB"
`drawCeilingLine()` generates the label as:
```cpp
juce::String label = juce::String (ceilingDB_, 1) + "dB";
// Result: "-0.1dB" — no space between number and unit
```
The reference shows a properly spaced label like "-0.1 dB". Fix: insert a space.

### Issue B: Grid labels use abbreviated "-3" style without leading space padding
When the scale labels ("-3", "-6", etc.) are drawn, they are left-aligned in a narrow area but can appear cramped. The reference places labels closer to the tick marks and with consistent alignment. Verify and adjust `juce::Justification::centredLeft` vs `centred` if the labels look misaligned at small sizes.

Reference frames:
- `/reference-docs/video-frames/v1-0040.png` — close-up of waveform scale shows label format
- `/reference-docs/video-frames/v1-0025.png` — detailed metering view

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawCeilingLine()` (line ~290) and `drawScale()` (line ~478)

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, screenshot — expected: ceiling label reads "-0.1 dB" (with space) not "-0.1dB"
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details

In `WaveformDisplay.cpp::drawCeilingLine()`, change:
```cpp
// BEFORE:
juce::String label = juce::String (ceilingDB_, 1) + "dB";

// AFTER:
juce::String label = juce::String (ceilingDB_, 1) + " dB";
```

Also consider adding the "dB" suffix to scale labels in `drawScale()` to match the reference format (where grid marks show "-3 dB", "-6 dB" etc.), but this is secondary — only add if the reference clearly confirms this (check v1-0040.png carefully).

## Dependencies
Requires task 185 (both modify WaveformDisplay.cpp — dead code removal lands first)
