# Task: Extract scattered UI magic numbers into named constants

## Description
Multiple UI components use hardcoded numeric values for dimensions, radii, alpha values, and zone boundaries instead of named constants. This makes tuning difficult and obscures intent. Key offenders:

1. **RotaryKnob.cpp**: `0.80f` face radius ratio, `3.5f` pointer thickness, label heights `10.0f`
2. **LevelMeter.cpp**: `0.05f`/`0.15f` danger/warn offset, `4.0f` clip box height, various alpha values
3. **WaveformDisplay.cpp**: Zone boundary percentages (`0.56f`, `0.36f`, `0.58f`, `0.82f`, etc.)
4. **ControlStrip.cpp**: `-12` pixel offsets for labels, separator positions
5. **GainReductionMeter.cpp** and **LevelMeter.cpp**: duplicate `kSegH = 3.0f`, `kSegGap = 1.0f`

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — add shared dimensional constants (segment sizes, etc.)
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — extract local constants for magic multipliers
Modify: `M-LIM/src/ui/LevelMeter.cpp` — extract local constants
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — extract zone boundary constants
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — use shared segment constants from Colours.h

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: `grep -rn "kSegH\|kSegGap" M-LIM/src/ui/Colours.h` → Expected: shared constants defined

## Tests
None (cosmetic refactor, no behavior change)

## Technical Details
- Move `kSegH` and `kSegGap` to `Colours.h` as `constexpr float` under the MLIMColours namespace
- In each component file, define local `constexpr` for component-specific magic numbers with descriptive names (e.g., `kFaceRadiusRatio = 0.80f`, `kPointerThickness = 3.5f`)
- For WaveformDisplay zone boundaries, group them in a local constants block with comments explaining what each zone represents

## Dependencies
None
