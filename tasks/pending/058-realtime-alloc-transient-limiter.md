# Task 058: Eliminate Per-Sample Vector Allocation in TransientLimiter::process()

## Description
TransientLimiter::process() allocates a `std::vector<float>` on the heap **per sample** at line 180:
```cpp
std::vector<float> perChRequiredGain(chCount, 1.0f);
```
This is inside the inner sample loop, meaning for a 512-sample stereo buffer it performs 512 heap allocations and deallocations per processBlock call. This is catastrophic for real-time audio performance and must be replaced with a pre-allocated member variable.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add pre-allocated member for per-channel gain
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — use pre-allocated member at line 180

## Acceptance Criteria
- [ ] Run: `grep -n "std::vector.*perChRequired" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: no matches (vector removed from inner loop)
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R transient --output-on-failure` → Expected: all transient limiter tests pass

## Tests
None (existing tests cover correctness; this is a real-time safety fix)

## Technical Details
**Fix:**
1. Add to TransientLimiter.h: `std::vector<float> mPerChRequiredGain;`
2. In prepare(): `mPerChRequiredGain.resize(numChannels, 1.0f);`
3. In process() line 180, replace the local vector with:
   ```cpp
   std::fill(mPerChRequiredGain.begin(), mPerChRequiredGain.end(), 1.0f);
   ```
   And use `mPerChRequiredGain` throughout the rest of the sample loop.

Alternative: since max channels is typically 2, a fixed-size `float perChRequiredGain[8]` (like LevelingLimiter uses) would also work and avoids any heap allocation.

## Dependencies
Requires task 011
