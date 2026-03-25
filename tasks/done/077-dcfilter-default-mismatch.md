# Task 077: DCFilter Default Mismatch — Engine Defaults to Enabled, Parameter Defaults to Disabled

## Description
`LimiterEngine.h` line 127 initialises the internal atomic flag:

```cpp
std::atomic<bool>  mDCFilterEnabled { true };   // engine default = ON
```

But `Parameters.cpp` line 94 declares the APVTS parameter with a default of `false`:

```cpp
params.push_back(std::make_unique<AudioParameterBool>(
    ParameterID { ParamID::dcFilterEnabled, 1 },
    "DC Filter",
    false   // parameter default = OFF
));
```

On plugin instantiation, `prepareToPlay` is called, which calls `pushAllParametersToEngine()`,
which will eventually push the correct `false` value. However:

1. If any audio arrives **before** the first `processBlock` (possible in some hosts during
   plugin scanning), the DC filter will be active when the user expects it off.
2. More importantly, every time `processBlock` runs it reads the atomic directly via
   `mDCFilterEnabled.load()`. The value is set correctly in `processBlock` via
   `limiterEngine.setDCFilterEnabled(...)`. But the mismatch is a latent foot-gun: if
   `pushAllParametersToEngine` is ever skipped or called out of order, the engine silently
   applies DC filtering to audio.
3. The inconsistency makes it harder to reason about initial state and is a subtle source of
   future bugs.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.h` — change `mDCFilterEnabled` initialiser from `true` to `false`

## Acceptance Criteria
- [ ] Run: `grep "mDCFilterEnabled" M-LIM/src/dsp/LimiterEngine.h` → Expected: `{ false }` initialiser
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None — existing DSP tests will catch regressions.

## Technical Details
In `src/dsp/LimiterEngine.h`, change:

```cpp
std::atomic<bool>  mDCFilterEnabled { true };
```
to:
```cpp
std::atomic<bool>  mDCFilterEnabled { false };
```

No other changes needed. The parameter default (`false`) already matches what users see in the
UI. The fix aligns the engine's cold-start state with the parameter default.

## Dependencies
None
