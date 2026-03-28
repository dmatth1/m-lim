# Task 542: GR Meter Width Increase

## Description
The gain reduction meter between waveform and right panel is only 6px wide (kGRMeterW = 6). The Pro-L 2 reference shows approximately 8-10px. Increase to 8px.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — line 79: change `kGRMeterW = 6` to `kGRMeterW = 8`

## Acceptance Criteria
- [ ] Run: `grep -n 'kGRMeterW' M-LIM/src/PluginEditor.h` → Expected: `kGRMeterW = 8`
- [ ] Run: `cd M-LIM && cmake --build build --config Release --target M-LIM_Standalone -j$(nproc)` → Expected: builds successfully

## Tests
None

## Dependencies
None
