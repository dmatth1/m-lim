# Task 059: Eliminate Audio-Thread Allocations and Unbounded Growth in LoudnessMeter

## Description
Two real-time safety issues in LoudnessMeter:

### Issue 1: Heap allocations in updateIntegratedAndLRA() (called from audio thread)
- **Line 188**: `std::vector<double> windowPowers` — allocated every 100ms block completion
- **Line 254**: `std::vector<float> stLoudness` — allocated every 100ms block completion
These vectors are built from mGatedBlockHistory and grow proportionally to recording duration.

### Issue 2: Unbounded memory growth in mGatedBlockHistory
- **Line 101 (header), line 171 (.cpp)**: `mGatedBlockHistory.push_back(blockMeanSquare)` grows indefinitely
- After 1 hour of audio at 48kHz: 36,000 blocks × 8 bytes = 288KB (manageable)
- After 24 hours: ~7MB (getting concerning)
- The real problem: updateIntegratedAndLRA() iterates ALL history entries multiple times per 100ms, causing O(n²) CPU growth over time. After hours of playback, this will cause audio dropouts.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — add pre-allocated working buffers; consider circular buffer for history
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — pre-allocate vectors in prepare(), cap or optimize history iteration

## Acceptance Criteria
- [ ] Run: `grep -n "std::vector" M-LIM/src/dsp/LoudnessMeter.cpp | grep -v "^[0-9]*:.*reserve"` → Expected: no vector construction inside updateIntegratedAndLRA() or onBlockComplete()
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R loudness --output-on-failure` → Expected: all loudness meter tests pass

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_integrated_lufs_long_session` — process 30+ minutes of audio, verify integrated LUFS converges and processing time per block stays bounded

## Technical Details
**Fix for Issue 1:**
Pre-allocate windowPowers and stLoudness as member vectors in prepare(). Reserve a reasonable capacity (e.g., 36000 for 1 hour). Clear and reuse each call rather than allocating fresh.

**Fix for Issue 2 (recommended approach):**
The ITU-R BS.1770-4 integrated loudness algorithm technically requires all historical blocks. However, for practical plugin use:
1. Cap mGatedBlockHistory to a maximum (e.g., 360,000 blocks = 10 hours). After that, either:
   - Stop growing and use running statistics (maintain running sums for gated mean)
   - Use a circular buffer and approximate integrated LUFS
2. Better: maintain running accumulators (count, sum) for both gating passes rather than re-scanning all history each block. This converts O(n) per block to O(1).

**Running accumulator approach for integrated LUFS:**
- Track: totalAboveAbsGate (sum + count), and update relative gate periodically
- Relative gate only needs occasional recalculation (e.g., every 10 seconds)
- This matches how professional metering implementations handle long sessions

## Dependencies
Requires task 010
