# Task 068: LoudnessMeter — Audio Thread Real-Time Safety Violations

## Description
`LoudnessMeter::updateIntegratedAndLRA()` is called from `processBlock()` on the audio thread every 100ms. It has multiple real-time safety violations that worsen over time:

1. **Unbounded memory growth**: `mGatedBlockHistory` grows without limit. After 1 hour at 100ms blocks = 36,000 entries. After 8 hours = 288,000 entries.

2. **Heap allocations on audio thread**: Lines 188 and 254 create `std::vector<double> windowPowers` and `std::vector<float> stLoudness` on every call, with sizes proportional to session length.

3. **O(n²) computation**: Building `windowPowers` iterates `n * 4` elements (n = number of 100ms blocks since reset). At 36,000 blocks (1 hour), this is ~144,000 iterations every 100ms.

4. **Sorting on audio thread**: `std::sort(stLoudness.begin(), stLoudness.end())` at line 273 is O(n log n) on growing data.

5. **Data races**: `mMomentaryLUFS`, `mShortTermLUFS`, `mIntegratedLUFS`, `mLoudnessRange` are written on the audio thread and read from the UI thread via getters, without atomic or mutex protection.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — add atomic result fields, pre-allocated working buffers, restructure storage
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — use incremental algorithm, pre-allocate vectors in prepare(), cap or optimize history

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `grep -n "std::vector" M-LIM/src/dsp/LoudnessMeter.cpp | grep -v "mGated"` → Expected: no vector allocations inside processBlock/onBlockComplete/updateIntegratedAndLRA call chain
- [ ] Run: `grep -n "std::sort" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: no sorting in audio-thread code paths

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_long_session_no_alloc` — process >1000 blocks and verify no performance degradation (timing test)
- Unit: `tests/dsp/test_loudness_meter.cpp::test_integrated_lufs_long_session` — process 30+ minutes of audio, verify integrated LUFS converges and processing time per block stays bounded

## Technical Details
**Recommended approach**: Use incremental/running statistics instead of recalculating from scratch:

1. **Integrated LUFS**: Maintain running sums for the absolute-gated and relative-gated passes. When a new 400ms window completes, add it to the running sum if it passes the gate. Recompute the relative gate threshold periodically (e.g., every 10 blocks) rather than every block.

2. **LRA**: Use a histogram-based approach. Bin the short-term LUFS values into 0.1 LU bins (covering -70 to +20 LUFS = 900 bins). Finding the 10th and 95th percentiles from a histogram is O(bins), not O(n log n).

3. **Memory**: Cap `mGatedBlockHistory` to a maximum size or switch to running accumulators. For EBU R128, the standard allows time-limited integration.

4. **Data races**: Make result fields `std::atomic<float>` or use `juce::Atomic<float>`.

5. **Pre-allocate vectors**: Move windowPowers and stLoudness to member variables, reserve capacity in prepare(), clear and reuse each call.

## Dependencies
None
