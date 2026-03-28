# Task: Brighten GR Meter Track Background

## Description
The gain reduction meter track (narrow vertical bar between waveform and loudness panel) is too dark compared to Pro-L 2 reference. Pixel comparison: M-LIM GR meter track renders ~(42,40,56) while reference shows ~(72,72,89). The `barTrackBackground` constant (0xff2A2838 = 42,40,56) is shared between GR meter, input meter, and output meter; however, the GR meter area should appear brighter to match the reference's medium-dark purple-gray.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `barTrackBackground` (0xff2A2838) needs brightening to approximately `0xff484859` = RGB(72,72,89)
Read: `src/ui/GainReductionMeter.cpp` — uses barTrackBackground for meter track fill
Read: `src/ui/LevelMeter.cpp` — also uses barTrackBackground; verify change doesn't harm input/output meter appearance
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference GR meter area

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at GR meter track → Expected: pixel values closer to (72,72,89) than current (42,40,56)

## Tests
None

## Technical Details
- `barTrackBackground` is shared across GR meter, input meter, and output meter
- Changing this affects all three meters; verify the input meter grooming task (brighten-input-meter-idle-gradient) accounts for a brighter base background
- Current: `0xff2A2838` = RGB(42,40,56) — too dark by ~30 units per channel
- Target: approximately `0xff484859` = RGB(72,72,89) — matches reference
- The existing grooming task for input meter brightness may benefit from this change

## Dependencies
None
