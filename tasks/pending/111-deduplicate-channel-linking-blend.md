# Task 111: Channel-Linking Blend Logic Duplicated in TransientLimiter and LevelingLimiter

## Description
Both `TransientLimiter::process()` and `LevelingLimiter::process()` contain
identical channel-linking blend code:

**TransientLimiter.cpp lines 268–276:**
```cpp
if (chCount > 1 && mChannelLink > 0.0f)
{
    float minRequired = 1.0f;
    for (int ch = 0; ch < chCount; ++ch)
        minRequired = std::min(minRequired, perChRequiredGain[ch]);
    for (int ch = 0; ch < chCount; ++ch)
        perChRequiredGain[ch] = perChRequiredGain[ch] * (1.0f - mChannelLink)
                                + minRequired * mChannelLink;
}
```

**LevelingLimiter.cpp lines 151–159:** identical logic.

Keeping two copies means any change (e.g., adding mid-side linking) must be made
in both places.

Fix: Add the shared inline helper to `DspUtil.h` (created in task 109, or create
it here if task 109 is not yet done):
```cpp
/// Blend each element of perChGain toward the minimum across all channels.
/// @param perChGain  pointer to chCount gain values (modified in-place)
/// @param chCount    number of channels
/// @param link       blend factor 0=independent, 1=fully linked
inline void applyChannelLinking(float* perChGain, int chCount, float link) noexcept;
```
Replace both inline blocks with a call to `applyChannelLinking`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/DspUtil.h` — add applyChannelLinking (create file if absent)
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace inline block, add include
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — replace inline block, add include

## Acceptance Criteria
- [ ] Run: `grep -c "minRequired" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: 0
- [ ] Run: `grep -c "minRequired" M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: 0
- [ ] Run: `grep -c "applyChannelLinking" M-LIM/src/dsp/DspUtil.h` → Expected: 1
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds
- [ ] Run: `cd M-LIM && cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None — pure refactor; existing tests validate correctness.

## Technical Details
- Function takes a plain `float*` (the existing stack arrays are `float[8]`) so no
  container dependency is introduced.
- Keep `chCount > 1 && link > 0.0f` guard inside the helper so call sites are
  unconditional.

## Dependencies
None
