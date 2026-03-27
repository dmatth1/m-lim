# Task: Add compile-time/runtime bounds guard to TransientLimiter per-channel stack buffer

## Description
`TransientLimiter::process()` allocates a fixed-size stack buffer sized for up
to 8 channels:

```cpp
// src/dsp/TransientLimiter.cpp line 311
float perChRequiredGain[8];  // supports up to 8 channels (7.1 surround)
```

The effective channel count is clamped at the top of `process()` via
`std::min(numChannels, mNumChannels)`, and `mNumChannels` is set in `prepare()`.
However there is no assertion or compile-time guarantee that `mNumChannels` was
itself prepared with a value ≤ 8. If a host ever calls `prepareToPlay` with more
than 8 channels, this silently overflows the stack.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/TransientLimiter.cpp` — add guard
Modify: `src/dsp/TransientLimiter.h` — optionally expose the constant
Read: `src/dsp/TransientLimiter.cpp` — lines 245–260 (prepare), lines 307–315 (process)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `grep "jassert\|static_assert\|kMaxChannels" M-LIM/src/dsp/TransientLimiter.cpp | head -5` → Expected: at least one guard line is present

## Tests
None

## Technical Details
Two complementary guards:

1. In `prepare()` (or wherever `mNumChannels` is set), add:
   ```cpp
   static constexpr int kMaxChannels = 8;
   jassert(numChannels <= kMaxChannels);
   mNumChannels = std::min(numChannels, kMaxChannels);
   ```

2. In `process()`, replace the raw array with:
   ```cpp
   float perChRequiredGain[kMaxChannels];
   ```
   so the constant is defined once.

No behaviour change for valid input — the clamp already handles it; this makes
the safety boundary explicit and debuggable.

## Dependencies
None
