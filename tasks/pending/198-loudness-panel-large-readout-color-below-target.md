# Task 198: LoudnessPanel — Large LUFS Readout Uses Near-White When Below Target

## Description
`LoudnessPanel::drawLargeReadout()` selects the colour for the large numeric LUFS display as:

```cpp
const juce::Colour valColour = (shortTerm_ < targetLUFS_)
                             ? MLIMColours::textPrimary    // 0xffE0E0E0 — near-white
                             : MLIMColours::meterDanger;   // 0xffFF5252 — red
```

When loudness is below the target (the normal, acceptable state), the large number is near-white (`#E0E0E0`) — indistinguishable from static label text and lacking visual feedback about loudness level.

The reference Pro-L 2 metering panel (`prol2-metering.jpg`, `v1-0030.png`) shows the main LUFS numeric readout in a **warm golden-yellow** when the signal is at or near the target, turning red when over. This gives clear at-a-glance feedback: golden = good, red = over.

Fix: Introduce a `lufsReadoutGood` colour constant in `Colours.h` (warm golden-yellow, e.g. `0xffE8C040`) and use it for the below-target state in `drawLargeReadout()`.

Also consider adding a transitional colour for the "close to target" range (within 1 LU): use `meterWarning` (yellow, `0xffFFD54F`) when `shortTerm_` is within 1 LU below target, and `lufsReadoutGood` when further below.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — add `lufsReadoutGood` constant
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — update `drawLargeReadout()` to use the new constant
Read: `M-LIM/src/ui/LoudnessPanel.h` — layout of the large readout strip (kLargeReadoutH = 48px)
Read: `/reference-docs/reference-screenshots/prol2-metering.jpg` — reference for the main LUFS numeric colour

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-198-after.png" && stop_app` → Expected: large LUFS readout at bottom of loudness panel shows a warm golden-yellow colour (not near-white), distinct from surrounding label text; the "LUFS" unit label below it remains in textSecondary grey
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (colour-only change)

## Technical Details

### Colours.h — add constant
```cpp
// After existing meter colours:
const juce::Colour lufsReadoutGood  { 0xffE8C040 };  // warm golden-yellow for LUFS readout (below target)
```

### LoudnessPanel.cpp — update drawLargeReadout()
```cpp
// BEFORE:
const juce::Colour valColour = (shortTerm_ < targetLUFS_)
                             ? MLIMColours::textPrimary
                             : MLIMColours::meterDanger;

// AFTER (with close-to-target transition):
juce::Colour valColour;
if (shortTerm_ >= targetLUFS_)
    valColour = MLIMColours::meterDanger;                         // over target: red
else if (shortTerm_ >= targetLUFS_ - 1.0f)
    valColour = MLIMColours::meterWarning;                        // within 1 LU of target: yellow
else
    valColour = MLIMColours::lufsReadoutGood;                     // comfortably below: golden
```

This mirrors how Pro-L 2 provides visual feedback about the current loudness level relative to the target.

## Dependencies
None
