# Task: GR Meter Width Increase

## Description
The gain reduction meter between the waveform and the right panel is only 6px wide (`kGRMeterW = 6`). In the Pro-L 2 reference, this separator/meter area is approximately 8-10px wide, providing a more visible visual break between the waveform and the loudness panel. Increasing to 8px will improve the left-region crop visual match.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/PluginEditor.h` — change `kGRMeterW = 6` to `kGRMeterW = 8`
Read: `src/ui/PluginEditor.cpp` — verify layout uses this constant in `resized()`

## Acceptance Criteria
- [ ] Run: `grep -n 'kGRMeterW' src/ui/PluginEditor.h` → Expected: shows value of 8
- [ ] Run: Build standalone, launch headless → Expected: visible slightly wider separator between waveform and right panel

## Tests
None

## Technical Details
Single constant change. The GR meter paints its background with `grMeterBackground` colour and draws segment separators. The extra 2px will make this visual separator more prominent matching the reference.

## Dependencies
None
