# Task: Extract `PeakHoldState` Struct from `PluginEditor` to Reduce Duplication

## Description

`PluginEditor` manages peak-hold state for two stereo meters using 8 separate member variables:

```cpp
float inputPeakL_  = -96.0f;   float inputPeakR_  = -96.0f;
float outputPeakL_ = -96.0f;   float outputPeakR_ = -96.0f;
int   inputPeakHoldFramesL_  = 0;  int inputPeakHoldFramesR_  = 0;
int   outputPeakHoldFramesL_ = 0;  int outputPeakHoldFramesR_ = 0;
```

The `updatePeakHold()` and `agePeakHoldCounters()` functions operate on these through a 6-parameter free function and a 4-call lambda pattern. The logic is identical for input and output meters.

This structure means adding a third meter (e.g., a sidechain or GR peak meter) requires duplicating all 4 variables and manually extending both `updatePeakHold()` and `agePeakHoldCounters()`.

**Fix:** Introduce a small `PeakHoldState` struct (in `PluginEditor.h` or a new header) that encapsulates the per-channel state and the update/aging logic:

```cpp
struct PeakHoldState {
    float peakL = -96.0f, peakR = -96.0f;
    int   framesL = 0,    framesR = 0;
    void update(float newL, float newR, int holdFrames) noexcept;
    void age() noexcept;
};
```

Replace the 8 member variables with `PeakHoldState inputPeak_, outputPeak_`. Replace the current `updatePeakHold()` / `agePeakHoldCounters()` free functions with calls to `inputPeak_.update(...)`, `inputPeak_.age()`, etc.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — replace 8 individual peak state variables with 2 `PeakHoldState` members; add struct definition (or Create a new `M-LIM/src/ui/PeakHoldState.h`); remove `updatePeakHold()` and `agePeakHoldCounters()` declarations
Modify: `M-LIM/src/PluginEditor.cpp` — update `timerCallback()`, remove old `updatePeakHold()` and `agePeakHoldCounters()` implementations, inline struct method calls

## Acceptance Criteria
- [ ] Run: `grep -c "inputPeakL_\|inputPeakR_\|outputPeakL_\|outputPeakR_\|inputPeakHoldFrames\|outputPeakHoldFrames" M-LIM/src/PluginEditor.h M-LIM/src/PluginEditor.cpp` → Expected: 0 (all individual variables replaced by struct)
- [ ] Run: `cd /workspace/M-LIM/build && cmake --build . --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -5` → Expected: exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (this is a refactor with identical external behaviour; existing integration tests verify meter data flow)

## Technical Details
- `PeakHoldState::update(newL, newR, holdFrames)`: if `newL > peakL`, set `peakL = newL` and `framesL = holdFrames`; else if `framesL > 0`, decrement `framesL`; else set `peakL = newL`. Same for R. This matches the existing lambda logic in `agePeakHoldCounters`.
- `PeakHoldState::age()`: call the decay logic for both channels (decrement frames, reset peak to current level when frames hit 0).
- `kPeakHoldFrames = 120` stays as a constexpr in `PluginEditor.h` and is passed to `update()`.
- If `PeakHoldState` is placed in its own header, add it to the CMakeLists `SOURCES` list.

## Dependencies
None
