# Task 426: Input Meter and GR Meter Track Background Brighten

## Description
The input level meter strip (x=0–30) and GR meter strip (x=648–660) are dramatically
darker than the reference at idle.

**Input meter (x=0–30, y=40–400):**
- Reference: `#625D6F` (R=98, G=93, B=111)
- M-LIM current: `#2E2D2A` (R=46, G=45, B=42)
- Gap: ~52 units darker

**GR meter strip (x=648–660, y=40–400):**
- Reference: `#6B7089` (R=107, G=112, B=137)
- M-LIM current: `#32262A` (R=50, G=38, B=42)
- Gap: ~57–75 units darker

Both use `MLIMColours::barTrackBackground = 0xff231417` (35,20,23). Lighten this to
bring idle track color closer to the waveform display background.

Target: `barTrackBackground` → `0xff2A2838` (dark blue-grey: R=42, G=40, B=56).

Note: `barTrackBackground` is shared by LevelMeter, GainReductionMeter, and LoudnessPanel.
Verify active meter fill still has visible contrast against the brighter background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `barTrackBackground` constant (~line 15)
Read: `src/ui/LevelMeter.cpp` — uses barTrackBackground (~line 76)
Read: `src/ui/GainReductionMeter.cpp` — uses barTrackBackground (~lines 82–104)
Read: `src/ui/LoudnessPanel.cpp` — uses barTrackBackground (~line 499)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: pixel check on input meter `convert screenshot.png -crop 30x360+0+40 +repage -resize 1x1! txt:-` → Expected: composite R ≥ 55 (up from 46)
- [ ] Run: pixel check on GR meter `convert screenshot.png -crop 12x360+648+40 +repage -resize 1x1! txt:-` → Expected: composite R ≥ 60 (up from 50)
- [ ] Visual: output meter and GR active fill have visible contrast against brighter track background

## Tests
None

## Technical Details
In `Colours.h`:
```cpp
// Current (too dark):
const juce::Colour barTrackBackground { 0xff231417 };
// Target (dark blue-grey, warmer for reference match):
const juce::Colour barTrackBackground { 0xff2A2838 };
```

Adjust in steps of ~8 units if target is too bright or too dark. Build Standalone:
`cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
