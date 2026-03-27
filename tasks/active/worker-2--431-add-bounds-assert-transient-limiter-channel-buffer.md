# Task 431: Add Bounds Guard to TransientLimiter Per-Channel Stack Buffer

## Description
`TransientLimiter::process()` allocates a fixed-size stack buffer for up to 8 channels:

```cpp
// src/dsp/TransientLimiter.cpp line 311
float perChRequiredGain[8];  // supports up to 8 channels (7.1 surround)
```

`mNumChannels` is clamped in `process()` via `std::min(numChannels, mNumChannels)`, but
there is no assertion or compile-time guarantee that `mNumChannels` itself was prepared
with a value ≤ 8. A host calling `prepareToPlay` with > 8 channels would silently overflow
the stack.

Fix:
1. Add a named constant `static constexpr int kMaxChannels = 8;` to replace the raw literal
2. Add `jassert(mNumChannels <= kMaxChannels)` in `prepare()` after setting `mNumChannels`
3. Change the stack buffer declaration to use the constant: `float perChRequiredGain[kMaxChannels]`

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/TransientLimiter.cpp` — add guard in prepare(), update buffer declaration (~lines 245–260, 307–315)
Modify: `src/dsp/TransientLimiter.h` — optionally expose the constant

## Acceptance Criteria
- [ ] Run: `grep -n "kMaxChannels\|jassert" src/dsp/TransientLimiter.cpp` → Expected: lines showing both the constant use and jassert
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
```cpp
// In TransientLimiter.h or .cpp:
static constexpr int kMaxChannels = 8;

// In prepare():
jassert (numChannels <= kMaxChannels);
mNumChannels = std::min (numChannels, kMaxChannels);

// In process():
float perChRequiredGain[kMaxChannels];
```

Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
