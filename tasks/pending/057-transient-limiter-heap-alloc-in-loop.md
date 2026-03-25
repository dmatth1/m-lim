# Task 057: TransientLimiter — Remove Per-Sample Heap Allocation in Process Loop

## Description
In `TransientLimiter::process()` at line 180, a `std::vector<float>` is allocated on every sample iteration:

```cpp
std::vector<float> perChRequiredGain(chCount, 1.0f);
```

This is a heap allocation inside the innermost per-sample loop — a severe real-time safety violation. At 48kHz stereo, this is 48,000 malloc/free pairs per second per channel. It causes unpredictable latency spikes and can trigger audio glitches.

The `LevelingLimiter` already uses the correct pattern: a stack-allocated `float perChRequiredGain[8]` array (line 129 of LevelingLimiter.cpp).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace `std::vector<float>` with stack array

## Acceptance Criteria
- [ ] Run: `grep -n "std::vector.*perChRequiredGain" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: no output (no vector allocation in process loop)
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (fix does not change behavior, only removes allocation)

## Technical Details
Replace line 180 in TransientLimiter.cpp:
```cpp
// BEFORE (heap allocation per sample — bad):
std::vector<float> perChRequiredGain(chCount, 1.0f);

// AFTER (stack allocation — good):
float perChRequiredGain[8];  // supports up to 8 channels
for (int ch = 0; ch < chCount; ++ch)
    perChRequiredGain[ch] = 1.0f;
```

The `chCount` is guaranteed ≤ `mNumChannels` which is set in `prepare()`. Supporting up to 8 channels (7.1 surround) is sufficient. This matches the pattern used in `LevelingLimiter::process()`.

## Dependencies
None
